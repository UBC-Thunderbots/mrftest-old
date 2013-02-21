#include "usb_bi_in.h"
#include "assert.h"
#include "minmax.h"
#include "registers.h"
#include "unused.h"
#include "usb_ep0.h"
#include "usb_ep0_sources.h"
#include "usb_fifo.h"
#include "usb_ll.h"

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
 *
 *
 *
 * The USB engine also has two other limitations:
 * 1. Except at the end of a packet, is only possible to push data into the transmit FIFO 4 bytes at a time.
 * 2. Reading from OTG_FS_DTXFSTSn when part, but not all, of a packet has already been pushed to the transmit FIFO causes packet truncation on the bus resulting in I/O errors.
 *
 * Limitation #1 is addressed by keeping a subword buffer in the endpoint info structure to hold up to 3 bytes provided by the application but not yet pushed to the FIFO.
 * This subword buffer is empty between packets, but may be nonempty when the application has only provided part of a packet so far.
 *
 * Limitation #2 is addressed by keeping track of how many bytes are left to push in the current packet and only checking for FIFO space at packet boundaries.
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

	// The number of bytes left in the current packet that have not been pushed into the FIFO yet.
	size_t packet_bytes_left_to_push;

	// Between zero and three bytes passed by the application but not yet pushed to the transmit FIFO because it must be written 4 bytes at a time.
	// In this buffer, bytes are shifted in from the MSB.
	uint32_t subword_buffer;

	// The number of bytes in subword_buffer.
	size_t subword_buffer_used;

	// Whether or not a ZLP will be sent after all data-carrying physical transfers for the current logical transfer are completed.
	bool zlp_pending;

	// A callback to invoke when the current logical transfer completes.
	void (*on_complete)(void);

	// A callback to invoke when more data is needed to continue the current logical transfer.
	void (*on_space)(void);

	// Whether this endpoint is using standard halt handling.
	bool standard_halt_handling;

	// Callbacks related to standard halt handling.
	usb_bi_in_enter_halt_cb_t enter_halt_cb;
	usb_bi_in_pre_exit_halt_cb_t pre_exit_halt_cb;
	usb_bi_in_post_exit_halt_cb_t post_exit_halt_cb;
};



static volatile uint32_t * const OTG_FS_DIEPCTL[] = { &OTG_FS_DIEPCTL0, &OTG_FS_DIEPCTL1, &OTG_FS_DIEPCTL2, &OTG_FS_DIEPCTL3 };
static volatile uint32_t * const OTG_FS_DIEPINT[] = { &OTG_FS_DIEPINT0, &OTG_FS_DIEPINT1, &OTG_FS_DIEPINT2, &OTG_FS_DIEPINT3 };
static volatile uint32_t * const OTG_FS_DIEPTSIZ[] = { &OTG_FS_DIEPTSIZ0, &OTG_FS_DIEPTSIZ1, &OTG_FS_DIEPTSIZ2, &OTG_FS_DIEPTSIZ3 };
static volatile uint32_t * const OTG_FS_DTXFSTS[] = { &OTG_FS_DTXFSTS0, &OTG_FS_DTXFSTS1, &OTG_FS_DTXFSTS2, &OTG_FS_DTXFSTS3 };

static struct ep_info ep_info[4] = {
	{ .state = USB_BI_IN_STATE_UNINITIALIZED, .standard_halt_handling = false, },
	{ .state = USB_BI_IN_STATE_UNINITIALIZED, .standard_halt_handling = false, },
	{ .state = USB_BI_IN_STATE_UNINITIALIZED, .standard_halt_handling = false, },
	{ .state = USB_BI_IN_STATE_UNINITIALIZED, .standard_halt_handling = false, },
};

static unsigned int standard_halt_handling_endpoint_count = 0;



static usb_ep0_disposition_t on_ep0_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_STD | USB_REQ_TYPE_ENDPOINT) && pkt->request == USB_REQ_CLEAR_FEATURE) {
		if (pkt->value == USB_FEATURE_ENDPOINT_HALT) {
			if (0x81 <= pkt->index && pkt->index <= 0x83 && ep_info[pkt->index & 0x7F].standard_halt_handling) {
				if (usb_bi_in_get_state(pkt->index & 0x7F) == USB_BI_IN_STATE_HALTED) {
					if (ep_info[pkt->index & 0x7F].pre_exit_halt_cb) {
						if (!ep_info[pkt->index & 0x7F].pre_exit_halt_cb(pkt->index & 0x7F)) {
							return USB_EP0_DISPOSITION_REJECT;
						}
					}
				}
				if (usb_bi_in_get_state(pkt->index & 0x7F) == USB_BI_IN_STATE_ACTIVE) {
					usb_bi_in_abort_transfer(pkt->index & 0x7F);
				}
				if (usb_bi_in_get_state(pkt->index & 0x7F) == USB_BI_IN_STATE_HALTED) {
					usb_bi_in_clear_halt(pkt->index & 0x7F);
				}
				usb_bi_in_reset_pid(pkt->index & 0x7F);
				if (ep_info[pkt->index & 0x7F].post_exit_halt_cb) {
					ep_info[pkt->index & 0x7F].post_exit_halt_cb(pkt->index & 0x7F);
				}
				return USB_EP0_DISPOSITION_ACCEPT;
			}
		}
	} else if (pkt->request_type == (USB_REQ_TYPE_STD | USB_REQ_TYPE_ENDPOINT) && pkt->request == USB_REQ_SET_FEATURE) {
		if (pkt->value == USB_FEATURE_ENDPOINT_HALT) {
			if (0x81 <= pkt->index && pkt->index <= 0x83 && ep_info[pkt->index & 0x7F].standard_halt_handling) {
				if (usb_bi_in_get_state(pkt->index & 0x7F) == USB_BI_IN_STATE_ACTIVE) {
					usb_bi_in_abort_transfer(pkt->index & 0x7F);
				}
				if (usb_bi_in_get_state(pkt->index & 0x7F) != USB_BI_IN_STATE_HALTED) {
					usb_bi_in_halt(pkt->index & 0x7F);
					if (ep_info[pkt->index & 0x7F].enter_halt_cb) {
						ep_info[pkt->index & 0x7F].enter_halt_cb(pkt->index & 0x7F);
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
		if (!pkt->value && pkt->length == 2 && 0x81 <= pkt->index && pkt->index <= 0x83 && ep_info[pkt->index & 0x7F].standard_halt_handling) {
			stash_buffer[0] = usb_bi_in_get_state(pkt->index & 0x7F) == USB_BI_IN_STATE_HALTED;
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
	// Sanity check.
	assert(ep_info[ep].state == USB_BI_IN_STATE_ACTIVE);
	assert(ep_info[ep].transfer_bytes_left_to_enable || ep_info[ep].zlp_pending);
	assert(!ep_info[ep].packet_bytes_left_to_push);
	assert(!ep_info[ep].subword_buffer_used);

	if (ep_info[ep].transfer_bytes_left_to_enable) {
		// We have some bytes remaining in the logical transfer.
		// We should construct a physical transfer that will send some or all of those bytes.
		// First, figure out how many bytes we will send in this transfer.

		// There is a limit on the number of packets we can send in this physical transfer; that limit is either eight or 1023 depending on whether the eight-packet workaround is needed.
		// In all cases, however, do not send more bytes than are left in the logical transfer.
		size_t bytes_this_transfer = MIN(ep_info[ep].transfer_bytes_left_to_enable, (ep_info[ep].eight_packet_workaround ? 8 : 1023) * ep_info[ep].max_packet);

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
	ep_info[ep].packet_bytes_left_to_push = 0;
	ep_info[ep].subword_buffer_used = 0;
	ep_info[ep].zlp_pending = false;
	ep_info[ep].on_complete = 0;
	ep_info[ep].on_space = 0;
	ep_info[ep].standard_halt_handling = false;
	ep_info[ep].enter_halt_cb = 0;
	ep_info[ep].pre_exit_halt_cb = 0;
	ep_info[ep].post_exit_halt_cb = 0;

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
	usb_ll_in_set_cb(ep, &handle_ep_interrupt);
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

	// Disable all interrupts for the endpoint.
	OTG_FS_DAINTMSK &= ~IEPM(1 << ep);
	OTG_FS_DIEPEMPMSK &= ~INEPTXFEM(1 << ep);

	// Deconfigure the endpoint.
	*OTG_FS_DIEPCTL[ep] = 0;

	// Unregister the callback.
	usb_ll_in_set_cb(ep, 0);

	// Update accounting.
	ep_info[ep].state = USB_BI_IN_STATE_UNINITIALIZED;
}

void usb_bi_in_set_std_halt(unsigned int ep, usb_bi_in_enter_halt_cb_t enter_halt_cb, usb_bi_in_pre_exit_halt_cb_t pre_exit_halt_cb, usb_bi_in_post_exit_halt_cb_t post_exit_halt_cb) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(ep_info[ep].state != USB_BI_IN_STATE_UNINITIALIZED);

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
	ep_info[ep].packet_bytes_left_to_push = 0;
	ep_info[ep].subword_buffer_used = 0;

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

size_t usb_bi_in_push(unsigned int ep, const void *data, size_t length) {
	// Sanity check.
	assert(ep);
	assert(ep <= 3);
	assert(length);
	assert(ep_info[ep].state == USB_BI_IN_STATE_ACTIVE);
	assert(ep_info[ep].transfer_bytes_left_to_push);

	// Get a source pointer going.
	const uint8_t *psrc = data;

	// Accumulate how much data we have consumed.
	size_t ret = 0;

	// Keep looping, trying to do something, until we run out of things to do.
	for (;;) {
		if (!length) {
			// The application is not making any more data available to us.
			// We can do nothing more.
			return ret;
		} else if (!ep_info[ep].transfer_bytes_left_to_push) {
			// The current physical transfer is fully satisfied with data in the FIFO.
			// Do not take FIFO-space-available interrupts any more for this endpoint; the FIFO will drain until transfer complete.
			OTG_FS_DIEPEMPMSK &= ~INEPTXFEM(1 << ep);
			// The application will have to try again later, after the transfer complete interrupt has been taken and a new physical transfer has started (if applicable).
			// We can do nothing more.
			return ret;
		} else if (!ep_info[ep].packet_bytes_left_to_push) {
			// We are not currently filling a packet, but we do need to add more packets to finish the current physical transfer.
			// Because we are not in the middle of a packet, it is OK to read OTG_FS_DTXFSTS.
			// We should do that now, to check if there is space in the FIFO for a whole packet.
			size_t fifo_space = INEPTFSAV_X(*OTG_FS_DTXFSTS[ep]) * 4;
			if (fifo_space < ep_info[ep].max_packet) {
				// There isn’t enough FIFO space for a single packet.
				// The application will have to try again later, probably in response to an on_space callback.
				// Wait until a full packet’s worth of space is available instead.
				// We can do nothing more.
				return ret;
			} else {
				// There is enough FIFO space for a packet.
				// Remember how much data is left to push in the current packet.
				// Note that this might be a short packet; we must set the byte count properly to account for that.
				ep_info[ep].packet_bytes_left_to_push = MIN(ep_info[ep].max_packet, ep_info[ep].transfer_bytes_left_to_push);
			}
		} else {
			// We want to push some data.
			// Keep going until we run out of either data to push or space in the current packet.
			while (ep_info[ep].subword_buffer_used + length > 0 && ep_info[ep].packet_bytes_left_to_push) {
				size_t to_push = MIN(4, ep_info[ep].packet_bytes_left_to_push);
				if (ep_info[ep].subword_buffer_used == to_push) {
					// We have accumulated enough bytes to satisfy either a full word OR the partial word at the end of the current packet and are ready to push that.
					// Remember that the subword buffer is populated from the MSB, so for a partial word push it must be shifted down so the bytes to send are at the LSB.
					OTG_FS_FIFO[ep][0] = ep_info[ep].subword_buffer >> (8 * (4 - to_push));
					ep_info[ep].subword_buffer_used = 0;
					ep_info[ep].transfer_bytes_left_to_push -= to_push;
					ep_info[ep].packet_bytes_left_to_push -= to_push;
				} else if (!length) {
					// There is SOME data in the subword buffer, but not enough to finish the current (full or partial) word.
					// We can’t add any data to the subword buffer, because there is no data left in the application buffer.
					// We have data, but we don’t have enough data to push, so we should stop here.
					return ret;
				} else {
					// We are not ready to push anything yet.
					// Accumulate more data into the subword buffer.
					ep_info[ep].subword_buffer = (ep_info[ep].subword_buffer >> 8) | (*psrc << 24);
					++ep_info[ep].subword_buffer_used;
					++psrc;
					--length;
					++ret;
				}
			}
		}
	}
}

