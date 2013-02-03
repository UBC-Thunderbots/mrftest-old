#include "usb_bi_out.h"
#include "assert.h"
#include "registers.h"
#include "usb.h"

/*
 * The USB engine in the STM32F4 has a number of numerical limitations:
 * 1. Only 19 bits are allocated to the Transfer Size field, so transfers can only be up to 524287 bytes.
 * 2. Only 10 bits are allocated to the Packet Count field, so transfers can only have up to 1023 packets.
 *
 * Limitation #1 is irrelevant because, this being the full-speed engine, packets can be no more than 64 bytes in length, so limitation #2 gives a maximum of 1023 × 64 = 65472 bytes anyway.
 *
 * Limitation #2, however, may be relevant.
 * An application could very well want to execute a single transfer that is larger than 1023 packets.
 * To support this possibility, we split up each application logical transfer into a series of smaller physical transfers in the USB engine.
 * The application submits logical transfers which can be up to 4294967295 bytes in length, regardless of packet size.
 * We internally split each logical transfer up into one or more physical transfers, each at most 1023 packets.
 */

struct ep_info {
	// The current state of the endpoint.
	usb_bi_out_state_t state;

	// The endpoint’s maximum packet size, in bytes.
	size_t max_packet;

	// The number of bytes left in the current logical transfer in physical transfers that have not started yet.
	size_t transfer_bytes_left_to_enable;

	// The number of bytes left in the current physical transfer.
	size_t transfer_bytes_left_to_read;

	// A callback to invoke when the current logical transfer completes.
	void (*on_complete)(void);

	// A callback to invoke when more a packet is received.
	void (*on_packet)(size_t);
};



static volatile uint32_t * const OTG_FS_DOEPCTL[] = { &OTG_FS_DOEPCTL0, &OTG_FS_DOEPCTL1, &OTG_FS_DOEPCTL2, &OTG_FS_DOEPCTL3 };
static volatile uint32_t * const OTG_FS_DOEPINT[] = { &OTG_FS_DOEPINT0, &OTG_FS_DOEPINT1, &OTG_FS_DOEPINT2, &OTG_FS_DOEPINT3 };
static volatile uint32_t * const OTG_FS_DOEPTSIZ[] = { &OTG_FS_DOEPTSIZ0, &OTG_FS_DOEPTSIZ1, &OTG_FS_DOEPTSIZ2, &OTG_FS_DOEPTSIZ3 };

static void handle_ep1_pattern(uint32_t);
static void handle_ep2_pattern(uint32_t);
static void handle_ep3_pattern(uint32_t);
static void (*const PATTERN_TRAMPOLINES[])(uint32_t) = { 0, &handle_ep1_pattern, &handle_ep2_pattern, &handle_ep3_pattern };

static struct ep_info ep_info[4] = {
	{ .state = USB_BI_OUT_STATE_UNINITIALIZED },
	{ .state = USB_BI_OUT_STATE_UNINITIALIZED },
	{ .state = USB_BI_OUT_STATE_UNINITIALIZED },
	{ .state = USB_BI_OUT_STATE_UNINITIALIZED },
};

static void start_physical_transfer(unsigned int ep) {
	assert(ep_info[ep].state == USB_BI_OUT_STATE_ACTIVE);
	assert(ep_info[ep].transfer_bytes_left_to_enable);

	// Compute the number of packets to expect.
	size_t num_packets = ep_info[ep].transfer_bytes_left_to_enable / ep_info[ep].max_packet;
	if (num_packets > 1023) {
		num_packets = 1023;
	}

	// Compute the maximum number of bytes that could be transferred if all these packets were maximum size.
	size_t max_bytes = num_packets * ep_info[ep].max_packet;

	// Compute the maximum number of bytes we expect to see in this physical transfer.
	size_t expected_max_bytes = max_bytes;
	if (expected_max_bytes > ep_info[ep].transfer_bytes_left_to_enable) {
		expected_max_bytes = ep_info[ep].transfer_bytes_left_to_enable;
	}

	// Set up the endpoint.
	*OTG_FS_DOEPTSIZ[ep] = PKTCNT(num_packets) | XFRSIZ(max_bytes);
	*OTG_FS_DOEPCTL[ep] |= EPENA | CNAK;

	// Update accounting.
	ep_info[ep].transfer_bytes_left_to_enable -= expected_max_bytes;
	ep_info[ep].transfer_bytes_left_to_read = expected_max_bytes;
}

static void handle_ep_pattern(unsigned int ep, uint32_t pattern) {
	assert(ep_info[ep].state == USB_BI_OUT_STATE_ACTIVE);

	if (PKTSTS_X(pattern) == 0b0010) {
		// A data packet arrived.
		size_t packet_size = BCNT_X(pattern);

		// Pass the packet to the application.
		if (packet_size) {
			ep_info[ep].on_packet(packet_size);
		}

		if (packet_size != ep_info[ep].max_packet) {
			// This is a short packet; mark that we should not run any more physical transfers in this logical transfer.
			ep_info[ep].transfer_bytes_left_to_read = 0;
			ep_info[ep].transfer_bytes_left_to_enable = 0;
		} else {
			// Update accounting.
			ep_info[ep].transfer_bytes_left_to_read -= packet_size;
		}
	} else if (PKTSTS_X(pattern) == 0b0011) {
		// A physical transfer is complete.
		if (ep_info[ep].transfer_bytes_left_to_enable) {
			// More bytes are left, so we need another physical transfer.
			start_physical_transfer(ep);
		} else {
			// We have reached the end of the logical transfer.
			// Notify the application.
			ep_info[ep].state = USB_BI_OUT_STATE_IDLE;
			assert(!ep_info[ep].transfer_bytes_left_to_read);
			assert(!ep_info[ep].transfer_bytes_left_to_enable);
			if (ep_info[ep].on_complete) {
				ep_info[ep].on_complete();
			}
		}
	}
}

static void handle_ep1_pattern(uint32_t pattern) {
	handle_ep_pattern(1, pattern);
}

static void handle_ep2_pattern(uint32_t pattern) {
	handle_ep_pattern(2, pattern);
}

static void handle_ep3_pattern(uint32_t pattern) {
	handle_ep_pattern(3, pattern);
}

usb_bi_out_state_t usb_bi_out_get_state(unsigned int ep) {
	assert(ep);
	assert(ep <= 3);
	return ep_info[ep].state;
}

void usb_bi_out_init(unsigned int ep, size_t max_packet, usb_bi_out_ep_type_t type) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(max_packet);
	assert(max_packet <= 64);
	assert(ep_info[ep].state == USB_BI_OUT_STATE_UNINITIALIZED);

	// Initialize data structure.
	ep_info[ep].state = USB_BI_OUT_STATE_IDLE;
	ep_info[ep].max_packet = max_packet;
	ep_info[ep].transfer_bytes_left_to_enable = 0;
	ep_info[ep].transfer_bytes_left_to_read = 0;
	ep_info[ep].on_complete = 0;
	ep_info[ep].on_packet = 0;

	// Enable the endpoint.
	*OTG_FS_DOEPCTL[ep] = SD0PID | SNAK | EPTYP(type) | USBAEP | MPSIZ(max_packet);

	// Wait until NAK status is effective.
	while (!(*OTG_FS_DOEPCTL[ep] & NAKSTS));

	// Register a callback to handle endpoint patterns for this endpoint.
	usb_out_set_callback(ep, PATTERN_TRAMPOLINES[ep]);
}

void usb_bi_out_deinit(unsigned int ep) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);

	// If a transfer is in progress, abort it.
	if (ep_info[ep].state == USB_BI_OUT_STATE_ACTIVE) {
		usb_bi_out_abort_transfer(ep);
	}

	// If the endpoint is halted, clear the halt.
	if (ep_info[ep].state == USB_BI_OUT_STATE_HALTED) {
		usb_bi_out_clear_halt(ep);
	}

	// Deconfigure the endpoint.
	*OTG_FS_DOEPCTL[ep] = 0;

	// Unregister the callback.
	usb_out_set_callback(ep, 0);

	// Update accounting.
	ep_info[ep].state = USB_BI_OUT_STATE_UNINITIALIZED;
}

void usb_bi_out_halt(unsigned int ep) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state == USB_BI_OUT_STATE_IDLE);

	// Do it.
	*OTG_FS_DOEPCTL[ep] |= DOEPCTL_STALL;
	ep_info[ep].state = USB_BI_OUT_STATE_HALTED;
}

void usb_bi_out_clear_halt(unsigned int ep) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state == USB_BI_OUT_STATE_HALTED);

	// Do it.
	*OTG_FS_DOEPCTL[ep] &= ~DOEPCTL_STALL;
	ep_info[ep].state = USB_BI_OUT_STATE_IDLE;
}

void usb_bi_out_reset_pid(unsigned int ep, unsigned int pid) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(pid <= 1);

	// Do it.
	*OTG_FS_DOEPCTL[ep] |= pid ? SD1PID : SD0PID;
}

void usb_bi_out_start_transfer(unsigned int ep, size_t max_length, void (*on_complete)(void), void (*on_packet)(size_t)) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(max_length);
	assert(on_packet);
	assert(ep_info[ep].state == USB_BI_OUT_STATE_IDLE);

	// Record callbacks.
	ep_info[ep].on_complete = on_complete;
	ep_info[ep].on_packet = on_packet;

	// Record how much data is left.
	ep_info[ep].transfer_bytes_left_to_enable = max_length;
	ep_info[ep].transfer_bytes_left_to_read = 0;

	// Mark the transfer as running.
	ep_info[ep].state = USB_BI_OUT_STATE_ACTIVE;

	// Get a physical transfer going.
	start_physical_transfer(ep);
}

void usb_bi_out_abort_transfer(unsigned int ep) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state == USB_BI_OUT_STATE_ACTIVE);

	// First get to the point where we are NAKing.
	*OTG_FS_DOEPCTL[ep] = (*OTG_FS_DOEPCTL[ep] | SNAK) & ~CNAK;
	while (!(*OTG_FS_DOEPCTL[ep] & NAKSTS));

	// Now shut down the transfer altogether.
	*OTG_FS_DOEPCTL[ep] |= EPDIS | SNAK;
	while (*OTG_FS_DOEPCTL[ep] & EPENA);

	// Update accounting.
	ep_info[ep].state = USB_BI_OUT_STATE_IDLE;
}

void usb_bi_out_read(unsigned int ep, void *dst, size_t length) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state == USB_BI_OUT_STATE_ACTIVE);
	assert(length);

	uint8_t *pdst = dst;

	// Read all the full words.
	size_t num_full_words = length / 4;
	while (num_full_words--) {
		uint32_t word = OTG_FS_FIFO[0][0];
		if (pdst) {
			*pdst++ = word;
			*pdst++ = word >> 8;
			*pdst++ = word >> 16;
			*pdst++ = word >> 24;
		}
	}

	// Read a partial word.
	size_t num_extra_bytes = length % 4;
	if (num_extra_bytes) {
		uint32_t word = OTG_FS_FIFO[0][0];
		if (pdst) {
			while (num_extra_bytes--) {
				*pdst++ = word;
				word >>= 8;
			}
		}
	}
}

