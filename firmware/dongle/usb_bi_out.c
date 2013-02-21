#include "usb_bi_out.h"
#include "assert.h"
#include "minmax.h"
#include "registers.h"
#include "unused.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"
#include "usb_ll.h"

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

	// The number of bytes left in the FIFO from the current packet.
	size_t packet_fifo_bytes_left;

	// The number of bytes left in the buffer word.
	size_t buffer_word_bytes_left;

	// Between 0 and 3 bytes read from the FIFO but not yet delivered to the application.
	uint32_t buffer_word;

	// A callback to invoke when the current logical transfer completes.
	void (*on_complete)(void);

	// A callback to invoke when more a packet is received.
	void (*on_packet)(size_t);

	// Whether this endpoint is using standard halt handling.
	bool standard_halt_handling;

	// Callbacks related to standard halt handling.
	usb_bi_out_enter_halt_cb_t enter_halt_cb;
	usb_bi_out_pre_exit_halt_cb_t pre_exit_halt_cb;
	usb_bi_out_post_exit_halt_cb_t post_exit_halt_cb;
};



static volatile uint32_t * const OTG_FS_DOEPCTL[] = { &OTG_FS_DOEPCTL0, &OTG_FS_DOEPCTL1, &OTG_FS_DOEPCTL2, &OTG_FS_DOEPCTL3 };
static volatile uint32_t * const OTG_FS_DOEPINT[] = { &OTG_FS_DOEPINT0, &OTG_FS_DOEPINT1, &OTG_FS_DOEPINT2, &OTG_FS_DOEPINT3 };
static volatile uint32_t * const OTG_FS_DOEPTSIZ[] = { &OTG_FS_DOEPTSIZ0, &OTG_FS_DOEPTSIZ1, &OTG_FS_DOEPTSIZ2, &OTG_FS_DOEPTSIZ3 };

static struct ep_info ep_info[4] = {
	{ .state = USB_BI_OUT_STATE_UNINITIALIZED, .standard_halt_handling = false, },
	{ .state = USB_BI_OUT_STATE_UNINITIALIZED, .standard_halt_handling = false, },
	{ .state = USB_BI_OUT_STATE_UNINITIALIZED, .standard_halt_handling = false, },
	{ .state = USB_BI_OUT_STATE_UNINITIALIZED, .standard_halt_handling = false, },
};

static unsigned int standard_halt_handling_endpoint_count = 0;



static usb_ep0_disposition_t on_ep0_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_STD | USB_REQ_TYPE_ENDPOINT) && pkt->request == USB_REQ_CLEAR_FEATURE) {
		if (pkt->value == USB_FEATURE_ENDPOINT_HALT) {
			if (0x01 <= pkt->index && pkt->index <= 0x03 && ep_info[pkt->index].standard_halt_handling) {
				if (usb_bi_out_get_state(pkt->index) == USB_BI_OUT_STATE_HALTED) {
					if (ep_info[pkt->index].pre_exit_halt_cb) {
						if (!ep_info[pkt->index].pre_exit_halt_cb(pkt->index)) {
							return USB_EP0_DISPOSITION_REJECT;
						}
					}
				}
				if (usb_bi_out_get_state(pkt->index) == USB_BI_OUT_STATE_ACTIVE) {
					usb_bi_out_abort_transfer(pkt->index);
				}
				if (usb_bi_out_get_state(pkt->index) == USB_BI_OUT_STATE_HALTED) {
					usb_bi_out_clear_halt(pkt->index);
				}
				usb_bi_out_reset_pid(pkt->index, 0);
				if (ep_info[pkt->index].post_exit_halt_cb) {
					ep_info[pkt->index].post_exit_halt_cb(pkt->index);
				}
				return USB_EP0_DISPOSITION_ACCEPT;
			}
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_STD | USB_REQ_TYPE_ENDPOINT) && pkt->request == USB_REQ_SET_FEATURE) {
		if (pkt->value == USB_FEATURE_ENDPOINT_HALT) {
			if (0x01 <= pkt->index && pkt->index <= 0x03 && ep_info[pkt->index].standard_halt_handling) {
				if (usb_bi_out_get_state(pkt->index) == USB_BI_OUT_STATE_ACTIVE) {
					usb_bi_out_abort_transfer(pkt->index);
				}
				if (usb_bi_out_get_state(pkt->index) != USB_BI_OUT_STATE_HALTED) {
					usb_bi_out_halt(pkt->index);
					if (ep_info[pkt->index].enter_halt_cb) {
						ep_info[pkt->index].enter_halt_cb(pkt->index);
					}
				}
				return USB_EP0_DISPOSITION_ACCEPT;
			}
		}
	}

	return USB_EP0_DISPOSITION_NONE;
}

static usb_ep0_disposition_t on_ep0_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static uint8_t stash_buffer[2];
	static usb_ep0_memory_source_t mem_src;

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_ENDPOINT) && pkt->request == USB_REQ_GET_STATUS) {
		if (!pkt->value && pkt->length == 2 && 0x01 <= pkt->index && pkt->index <= 0x03 && ep_info[pkt->index].standard_halt_handling) {
			stash_buffer[0] = usb_bi_out_get_state(pkt->index) == USB_BI_OUT_STATE_HALTED;
			stash_buffer[1] = 0;
			*source = usb_ep0_memory_source_init(&mem_src, stash_buffer, 2);
			return USB_EP0_DISPOSITION_ACCEPT;
		}
	}

	return USB_EP0_DISPOSITION_NONE;
}

static const usb_ep0_cbs_t EP0_CBS = {
	.on_zero_request = &on_ep0_zero_request,
	.on_in_request = &on_ep0_in_request,
	.on_out_request = 0,
};

static void start_physical_transfer(unsigned int ep) {
	assert(ep_info[ep].state == USB_BI_OUT_STATE_ACTIVE);
	assert(ep_info[ep].transfer_bytes_left_to_enable);
	assert(!ep_info[ep].packet_fifo_bytes_left);
	assert(!ep_info[ep].buffer_word_bytes_left);

	// Compute the number of packets to expect.
	size_t num_packets = MIN(1023, ep_info[ep].transfer_bytes_left_to_enable / ep_info[ep].max_packet);

	// Compute the maximum number of bytes that could be transferred if all these packets were maximum size.
	size_t max_bytes = num_packets * ep_info[ep].max_packet;

	// Compute the maximum number of bytes we expect to see in this physical transfer.
	size_t expected_max_bytes = MIN(max_bytes, ep_info[ep].transfer_bytes_left_to_enable);

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

		// ZLPs should not be passed to the application; it is notified of end-of-transfer by a separate mechanism.
		if (packet_size) {
			// Pass the packet to the application, truncating to the expected transfer size in case the host sent more than we expected.
			// We do not need to include transfer_bytes_left_to_enable here, because all non-final physical transfers are enabled as a multiple of max packet.
			// Therefore, the final packet in a non-final physical transfer will see transfer_bytes_left_to_read == max_packet, and packet_size cannot be any larger than that.
			size_t expected_packet_size = MIN(packet_size, ep_info[ep].transfer_bytes_left_to_read);
			ep_info[ep].packet_fifo_bytes_left = packet_size;
			ep_info[ep].on_packet(expected_packet_size);

			// Throw away any data the application didn’t care about, including any data the host sent beyond the expected transfer size.
			usb_bi_out_discard(ep, ep_info[ep].packet_fifo_bytes_left + ep_info[ep].buffer_word_bytes_left);
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
	ep_info[ep].packet_fifo_bytes_left = 0;
	ep_info[ep].buffer_word_bytes_left = 0;
	ep_info[ep].on_complete = 0;
	ep_info[ep].on_packet = 0;
	ep_info[ep].standard_halt_handling = false;
	ep_info[ep].enter_halt_cb = 0;
	ep_info[ep].pre_exit_halt_cb = 0;
	ep_info[ep].post_exit_halt_cb = 0;

	// Enable the endpoint.
	*OTG_FS_DOEPCTL[ep] = SD0PID | SNAK | EPTYP(type) | USBAEP | MPSIZ(max_packet);

	// Wait until NAK status is effective.
	while (!(*OTG_FS_DOEPCTL[ep] & NAKSTS));

	// Register a callback to handle endpoint patterns for this endpoint.
	usb_ll_out_set_cb(ep, &handle_ep_pattern);
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

	// If standard halt handling is in use for the endpoint, disable it and account accordingly.
	if (ep_info[ep].standard_halt_handling) {
		ep_info[ep].standard_halt_handling = false;
		ep_info[ep].enter_halt_cb = 0;
		ep_info[ep].pre_exit_halt_cb = 0;
		ep_info[ep].post_exit_halt_cb = 0;
		if (!--standard_halt_handling_endpoint_count) {
			usb_ep0_cbs_remove(&EP0_CBS);
		}
	}

	// Deconfigure the endpoint.
	*OTG_FS_DOEPCTL[ep] = 0;

	// Unregister the callback.
	usb_ll_out_set_cb(ep, 0);

	// Update accounting.
	ep_info[ep].state = USB_BI_OUT_STATE_UNINITIALIZED;
}

void usb_bi_out_set_std_halt(unsigned int ep, usb_bi_out_enter_halt_cb_t enter_halt_cb, usb_bi_out_pre_exit_halt_cb_t pre_exit_halt_cb, usb_bi_out_post_exit_halt_cb_t post_exit_halt_cb) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state != USB_BI_OUT_STATE_UNINITIALIZED);

	// If standard halt handling was not already active for this endpoint, account and potentially register the callbacks.
	if (!ep_info[ep].standard_halt_handling) {
		if (!standard_halt_handling_endpoint_count++) {
			usb_ep0_cbs_push(&EP0_CBS);
		}
	}

	// Set variables.
	ep_info[ep].standard_halt_handling = true;
	ep_info[ep].enter_halt_cb = enter_halt_cb;
	ep_info[ep].pre_exit_halt_cb = pre_exit_halt_cb;
	ep_info[ep].post_exit_halt_cb = post_exit_halt_cb;
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
	assert(dst);
	assert(length <= ep_info[ep].buffer_word_bytes_left + ep_info[ep].packet_fifo_bytes_left);

	uint8_t *pdst = dst;

	// Take any data left in the buffer word first.
	while (ep_info[ep].buffer_word_bytes_left && length) {
		*pdst = ep_info[ep].buffer_word;
		++pdst;
		--length;

		ep_info[ep].buffer_word >>= 8;
		--ep_info[ep].buffer_word_bytes_left;
	}

	// Read all the full words from the FIFO.
	while (length >= 4) {
		uint32_t word = OTG_FS_FIFO[0][0];
		ep_info[ep].packet_fifo_bytes_left -= 4;

		pdst[0] = word;
		pdst[1] = word >> 8;
		pdst[2] = word >> 16;
		pdst[3] = word >> 24;
		pdst += 4;
		length -= 4;
	}

	// Read a partial word if necessary.
	if (length) {
		ep_info[ep].buffer_word = OTG_FS_FIFO[0][0];
		ep_info[ep].buffer_word_bytes_left = MIN(4, ep_info[ep].packet_fifo_bytes_left);
		ep_info[ep].packet_fifo_bytes_left -= ep_info[ep].buffer_word_bytes_left;

		while (length) {
			*pdst = ep_info[ep].buffer_word;
			++pdst;
			--length;

			ep_info[ep].buffer_word >>= 8;
			--ep_info[ep].buffer_word_bytes_left;
		}
	}
}

void usb_bi_out_discard(unsigned int ep, size_t length) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state == USB_BI_OUT_STATE_ACTIVE);
	assert(length <= ep_info[ep].buffer_word_bytes_left + ep_info[ep].packet_fifo_bytes_left);

	// Take any data left in the buffer word first.
	size_t buffer_word_bytes_to_discard = MIN(length, ep_info[ep].buffer_word_bytes_left);
	ep_info[ep].buffer_word_bytes_left -= buffer_word_bytes_to_discard;
	length -= buffer_word_bytes_to_discard;

	// Read all the full words from the FIFO.
	while (length >= 4) {
		(void) OTG_FS_FIFO[0][0];
		ep_info[ep].packet_fifo_bytes_left -= 4;
		length -= 4;
	}

	// Read a partial word if necessary.
	if (length) {
		ep_info[ep].buffer_word = OTG_FS_FIFO[0][0];
		ep_info[ep].buffer_word_bytes_left = MIN(4, ep_info[ep].packet_fifo_bytes_left);
		ep_info[ep].packet_fifo_bytes_left -= ep_info[ep].buffer_word_bytes_left;

		ep_info[ep].buffer_word_bytes_left -= length;
		length = 0;
	}
}

