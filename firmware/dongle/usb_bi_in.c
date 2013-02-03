#include "usb_bi_in.h"
#include "assert.h"
#include "registers.h"
#include "usb.h"
#include "usb_fifo.h"

/*
 * The USB engine in the STM32F4 has a number of numerical limitations:
 * 1. Only eight packets’ data can be pushed into the FIFO at a time.
 * 2. Only 19 bits are allocated to the Transfer Size field, so transfers can only be up to 524287 bytes.
 * 3. Only 10 bits are allocated to the Packet Count field, so transfers can only have up to 1023 packets.
 *
 * Limitation #1 is normally irrelevant due to the comment on usb_bi_in_init() that the FIFO should be no more than 8× the maximum packet size.
 * Thus we will run out of space in the FIFO before or at the same time as we run out of packet count, so preventing space overrun automatically prevents packet count overrun.
 * However, we check the FIFO size and if this condition is not met, set a flag that indicates a workaround is needed.
 * The workaround is to split each application logical transfer up into a series of smaller physical transfers in the USB engine, each of which is at most eight packets.
 *
 * Limitation #2 is irrelevant because, this being the full-speed engine, packets can be no more than 64 bytes in length, so limitation #3 gives a maximum of 1023 × 64 = 65472 bytes anyway.
 *
 * Limitation #3, however, may be relevant.
 * An application could very well want to execute a single transfer that is larger than 1023 packets.
 * To support this possibility, we split up each application logical transfer into a series of smaller physical transfers in the USB engine.
 * The application submits logical transfers which can be up to 4294967295 bytes in length, regardless of packet size.
 * We internally split each logical transfer up into one or more physical transfers, each at most 1023 packets.
 *
 * The logical/physical split also allows us to handle zero-length packets properly.
 * The USB engine requires that a ZLP be submitted as a separate physical transfer.
 * When the application submits a logical transfer that should end with a ZLP, we execute one extra physical transfer for the ZLP before considering the logical transfer to be complete.
 */

struct ep_info {
	// The current state of the endpoint.
	usb_bi_in_state_t state;

	// The endpoint’s maximum packet size, in bytes.
	size_t max_packet;

	// Whether or not this endpoint needs the 8-packet-maximum workaround.
	bool eight_packet_workaround;

	// The number of bytes left in the current logical transfer in physical transfers that have not started yet.
	size_t transfer_bytes_left_to_enable;

	// The number of bytes left in the current physical transfer that have not been pushed into the FIFO yet.
	size_t transfer_bytes_left_to_push;

	// Whether or not a ZLP will be sent after all data-carrying physical transfers for the current logical transfer are completed.
	bool zlp_pending;

	// A callback to invoke when the current logical transfer completes.
	void (*on_complete)(void);

	// A callback to invoke when more data is needed to continue the current logical transfer.
	void (*on_space)(void);
};



static volatile uint32_t * const OTG_FS_DIEPCTL[] = { &OTG_FS_DIEPCTL0, &OTG_FS_DIEPCTL1, &OTG_FS_DIEPCTL2, &OTG_FS_DIEPCTL3 };
static volatile uint32_t * const OTG_FS_DIEPINT[] = { &OTG_FS_DIEPINT0, &OTG_FS_DIEPINT1, &OTG_FS_DIEPINT2, &OTG_FS_DIEPINT3 };
static volatile uint32_t * const OTG_FS_DIEPTSIZ[] = { &OTG_FS_DIEPTSIZ0, &OTG_FS_DIEPTSIZ1, &OTG_FS_DIEPTSIZ2, &OTG_FS_DIEPTSIZ3 };
static volatile uint32_t * const OTG_FS_DTXFSTS[] = { &OTG_FS_DTXFSTS0, &OTG_FS_DTXFSTS1, &OTG_FS_DTXFSTS2, &OTG_FS_DTXFSTS3 };

static void handle_ep1_interrupt(void);
static void handle_ep2_interrupt(void);
static void handle_ep3_interrupt(void);
static void (*const INTERRUPT_TRAMPOLINES[])(void) = { 0, &handle_ep1_interrupt, &handle_ep2_interrupt, &handle_ep3_interrupt };

static struct ep_info ep_info[4] = {
	{ .state = USB_BI_IN_STATE_UNINITIALIZED },
	{ .state = USB_BI_IN_STATE_UNINITIALIZED },
	{ .state = USB_BI_IN_STATE_UNINITIALIZED },
	{ .state = USB_BI_IN_STATE_UNINITIALIZED },
};



static void start_physical_transfer(unsigned int ep) {
	// Sanity check.
	assert(ep_info[ep].state == USB_BI_IN_STATE_ACTIVE);
	assert(ep_info[ep].transfer_bytes_left_to_enable || ep_info[ep].zlp_pending);

	if (ep_info[ep].transfer_bytes_left_to_enable) {
		// We have some bytes remaining in the logical transfer.
		// We should construct a physical transfer that will send some or all of those bytes.
		// First, figure out how many bytes we will send in this transfer.

		// There is a limit on the number of packets we can send in this physical transfer; that limit is either eight or 1023 depending on whether the eight-packet workaround is needed.
		size_t bytes_this_transfer;
		if (ep_info[ep].eight_packet_workaround) {
			bytes_this_transfer = 8;
		} else {
			bytes_this_transfer = 1023;
		}
		bytes_this_transfer *= ep_info[ep].max_packet;

		// In all cases, however, do not send more bytes than are left in the logical transfer.
		if (bytes_this_transfer > ep_info[ep].transfer_bytes_left_to_enable) {
			bytes_this_transfer = ep_info[ep].transfer_bytes_left_to_enable;
		}

		// Set up the endpoint.
		*OTG_FS_DIEPTSIZ[ep] = PKTCNT((bytes_this_transfer + ep_info[ep].max_packet - 1) / ep_info[ep].max_packet) | XFRSIZ(bytes_this_transfer);
		*OTG_FS_DIEPCTL[ep] = (*OTG_FS_DIEPCTL[ep] | EPENA | CNAK) & ~DIEPCTL_STALL;

		// Enable the “space available” interrupt if the application has registered a callback for it.
		if (ep_info[ep].on_space) {
			OTG_FS_DIEPEMPMSK |= INEPTXFEM(1 << ep);
		}

		// Update accounting info.
		ep_info[ep].transfer_bytes_left_to_enable -= bytes_this_transfer;
		ep_info[ep].transfer_bytes_left_to_push = bytes_this_transfer;
	} else if (ep_info[ep].zlp_pending) {
		// There are no bytes remaining in the logical transfer, but we must send a ZLP.
		// Set up the endpoint.
		*OTG_FS_DIEPTSIZ[ep] = PKTCNT(1) | XFRSIZ(0);
		*OTG_FS_DIEPCTL[ep] = (*OTG_FS_DIEPCTL[ep] | EPENA | CNAK) & ~DIEPCTL_STALL;

		// Update accounting info.
		ep_info[ep].zlp_pending = false;
	}
}

static void handle_ep_interrupt(unsigned int ep) {
	if (*OTG_FS_DIEPINT[ep] & XFRC) {
		// The current physical transfer is complete.
		// Clear the interrupt.
		*OTG_FS_DIEPINT[ep] = XFRC;

		// Check if we need to start another physical transfer or if we should report completion of the logical transfer to the application.
		if (ep_info[ep].transfer_bytes_left_to_enable || ep_info[ep].zlp_pending) {
			// There are more bytes or a ZLP to send.
			// Start another physical transfer.
			start_physical_transfer(ep);
		} else {
			// The logical transfer is now completely finished.
			// Disable FIFO space interrupts, if they were enabled.
			OTG_FS_DIEPEMPMSK &= ~INEPTXFEM(1 << ep);

			// Update accounting.
			ep_info[ep].state = USB_BI_IN_STATE_IDLE;

			// Notify the application.
			if (ep_info[ep].on_complete) {
				ep_info[ep].on_complete();
			}
		}
	} else if ((*OTG_FS_DIEPINT[ep] & TXFE) && ep_info[ep].transfer_bytes_left_to_push && ep_info[ep].on_space) {
		// There is space in the FIFO, and there are bytes remaining to push in the current physical transfer.
		// Call the application so the bytes can be pushed.
		ep_info[ep].on_space();
	}
}

static void handle_ep1_interrupt(void) {
	handle_ep_interrupt(1);
}

static void handle_ep2_interrupt(void) {
	handle_ep_interrupt(2);
}

static void handle_ep3_interrupt(void) {
	handle_ep_interrupt(3);
}

usb_bi_in_state_t usb_bi_in_get_state(unsigned int ep) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);

	// Return the current state.
	return ep_info[ep].state;
}

void usb_bi_in_init(unsigned int ep, size_t max_packet, usb_bi_in_ep_type_t type) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(max_packet);
	assert(max_packet <= 64);
	assert(ep_info[ep].state == USB_BI_IN_STATE_UNINITIALIZED);

	// Initialize data structure.
	ep_info[ep].state = USB_BI_IN_STATE_IDLE;
	ep_info[ep].max_packet = max_packet;
	ep_info[ep].eight_packet_workaround = usb_fifo_get_size(ep) > max_packet * 8;
	ep_info[ep].transfer_bytes_left_to_enable = 0;
	ep_info[ep].transfer_bytes_left_to_push = 0;
	ep_info[ep].zlp_pending = false;
	ep_info[ep].on_complete = 0;
	ep_info[ep].on_space = 0;

	// Clear any pending transfer complete interrupt on this endpoint.
	*OTG_FS_DIEPINT[ep] = XFRC;

	// Do not, at this time, take FIFO empty interrupts for this endpoint.
	OTG_FS_DIEPEMPMSK &= ~INEPTXFEM(1 << ep);

	// Enable interrupts in general for this endpoint.
	OTG_FS_DAINTMSK |= IEPM(1 << ep);

	// Enable the endpoint.
	*OTG_FS_DIEPCTL[ep] = SD0PID | SNAK | DIEPCTL_TXFNUM(ep) | EPTYP(type) | USBAEP | MPSIZ(max_packet);

	// Wait until NAK status is effective.
	while (!(*OTG_FS_DIEPCTL[ep] & NAKSTS));

	// Register a callback to handle endpoint interrupts for this endpoint.
	usb_in_set_callback(ep, INTERRUPT_TRAMPOLINES[ep]);
}

void usb_bi_in_deinit(unsigned int ep) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);

	// If a transfer is in progress, abort it.
	if (ep_info[ep].state == USB_BI_IN_STATE_ACTIVE) {
		usb_bi_in_abort_transfer(ep);
	}

	// If the endpoint is halted, clear the halt.
	if (ep_info[ep].state == USB_BI_IN_STATE_HALTED) {
		usb_bi_in_clear_halt(ep);
	}

	// Disable all interrupts for the endpoint.
	OTG_FS_DAINTMSK &= ~IEPM(1 << ep);
	OTG_FS_DIEPEMPMSK &= ~INEPTXFEM(1 << ep);

	// Deconfigure the endpoint.
	*OTG_FS_DIEPCTL[ep] = 0;

	// Unregister the callback.
	usb_in_set_callback(ep, 0);

	// Update accounting.
	ep_info[ep].state = USB_BI_IN_STATE_UNINITIALIZED;
}

void usb_bi_in_halt(unsigned int ep) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state == USB_BI_IN_STATE_IDLE);

	// Do it.
	*OTG_FS_DIEPCTL[ep] |= DIEPCTL_STALL;
	ep_info[ep].state = USB_BI_IN_STATE_HALTED;
}

void usb_bi_in_clear_halt(unsigned int ep) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state == USB_BI_IN_STATE_HALTED);

	// Do it.
	*OTG_FS_DIEPCTL[ep] &= ~DIEPCTL_STALL;
	ep_info[ep].state = USB_BI_IN_STATE_IDLE;
}

void usb_bi_in_reset_pid(unsigned int ep) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state == USB_BI_IN_STATE_IDLE);

	// Do it.
	*OTG_FS_DIEPCTL[ep] |= SD0PID;
}

void usb_bi_in_start_transfer(unsigned int ep, size_t length, size_t max_length, void (*on_complete)(void), void (*on_space)(void)) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(length <= max_length);
	assert(ep_info[ep].state == USB_BI_IN_STATE_IDLE);

	// Record callbacks.
	ep_info[ep].on_complete = on_complete;
	ep_info[ep].on_space = on_space;

	// Compute whether we will need a ZLP.
	ep_info[ep].zlp_pending = length != max_length && !(length % ep_info[ep].max_packet);

	// Record how much data is left.
	ep_info[ep].transfer_bytes_left_to_enable = length;
	ep_info[ep].transfer_bytes_left_to_push = 0;

	// Mark the transfer as running.
	ep_info[ep].state = USB_BI_IN_STATE_ACTIVE;

	// Get a physical transfer going.
	start_physical_transfer(ep);
}

void usb_bi_in_abort_transfer(unsigned int ep) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state == USB_BI_IN_STATE_ACTIVE);

	// First get to the point where we are NAKing.
	*OTG_FS_DIEPINT[ep] = INEPNE;
	*OTG_FS_DIEPCTL[ep] = (*OTG_FS_DIEPCTL[ep] | SNAK) & ~CNAK;
	while (!(*OTG_FS_DIEPINT[ep] & INEPNE));

	// Now shut down the transfer altogether.
	*OTG_FS_DIEPCTL[ep] |= EPDIS | SNAK;
	while (*OTG_FS_DIEPCTL[ep] & EPENA);

	// It’s possible that before we entered this function, or while we were in the process of aborting, the current physical transfer finished.
	// Getting into the transfer complete interrupt handler with an idle endpoint would be bad.
	// Presumably the transfer complete interrupt should not be getting set right now, at this point, since the endpoint is presently disabled.
	// Thus, just in case the interrupt was already set before we got here, clear it now.
	*OTG_FS_DIEPINT[ep] = XFRC;

	// In case they were enabled, disable FIFO space available interrupts for this endpoint.
	OTG_FS_DIEPEMPMSK &= ~INEPTXFEM(1 << ep);

	// Update accounting.
	ep_info[ep].state = USB_BI_IN_STATE_IDLE;
}

bool usb_bi_in_push_word(unsigned int ep, uint32_t word) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state == USB_BI_IN_STATE_ACTIVE);
	assert(ep_info[ep].transfer_bytes_left_to_push);

	// Check for buffer space.
	if (!INEPTFSAV_X(*OTG_FS_DTXFSTS[ep])) {
		return false;
	}

	// Push the word.
	OTG_FS_FIFO[ep][0] = word;

	// Account.
	if (ep_info[ep].transfer_bytes_left_to_push <= 4) {
		ep_info[ep].transfer_bytes_left_to_push = 0;

		// We are reaching the end of the current physical transfer.
		// Do not take FIFO-space-available interrupts any more for this endpoint; the FIFO will drain until transfer complete.
		OTG_FS_DIEPEMPMSK &= ~INEPTXFEM(1 << ep);
	} else {
		ep_info[ep].transfer_bytes_left_to_push -= 4;
	}

	return true;
}

size_t usb_bi_in_push_block(unsigned int ep, const void *data, size_t length) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(length);
	assert(ep_info[ep].state == USB_BI_IN_STATE_ACTIVE);
	assert(ep_info[ep].transfer_bytes_left_to_push);

	// Get buffer space.
	size_t buffer_space = INEPTFSAV_X(*OTG_FS_DTXFSTS[ep]) * 4;
	if (!buffer_space) {
		return 0;
	}

	// Compute how much data to push.
	size_t to_push = ep_info[ep].transfer_bytes_left_to_push;
	if (length < to_push) {
		to_push = length;
	}
	if (buffer_space < to_push) {
		to_push = buffer_space;
	}

	// Update accounting info.
	ep_info[ep].transfer_bytes_left_to_push -= to_push;

	// Push the data.
	size_t words_to_push = (to_push + 3) / 4;
	const uint8_t *psrc = data;
	while (words_to_push--) {
		uint32_t word = psrc[0] | (psrc[1] << 8) | (psrc[2] << 16) | (psrc[3] << 24);
		OTG_FS_FIFO[ep][0] = word;
		psrc += 4;
	}

	// Check how much data is left to push on this physical transfer.
	if (!ep_info[ep].transfer_bytes_left_to_push) {
		// We are reaching the end of the current physical transfer.
		// Do not take FIFO-space-available interrupts any more for this endpoint; the FIFO will drain until transfer complete.
		OTG_FS_DIEPEMPMSK &= ~INEPTXFEM(1 << ep);
	}

	return to_push;
}

