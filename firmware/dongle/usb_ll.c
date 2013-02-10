#include "usb_ll.h"
#include "assert.h"
#include "rcc.h"
#include "registers.h"
#include "sleep.h"
#include "stdbool.h"

static usb_ll_state_t device_state = USB_LL_STATE_DETACHED;
static usb_ll_reset_cb_t reset_cb;
static usb_ll_enumeration_done_cb_t enumeration_done_cb;
static usb_ll_unplug_cb_t unplug_cb;

static usb_ll_in_cb_t in_endpoint_callbacks[4];
static usb_ll_out_cb_t out_endpoint_callbacks[4];

static usb_ll_gnak_req_t *gnak_requests_head = 0, *gnak_requests_tail = 0;
static bool gonak_requested, ginak_requested;



static void handle_reset_gnak(void) {
	// Sanity check.
	assert(device_state != USB_LL_STATE_DETACHED);

	// Notify the application.
	if (reset_cb) {
		reset_cb();
	}

	// Update state.
	device_state = USB_LL_STATE_ENUMERATING;

	// Enable enumeration complete and reset interrupts only, and clear any pending interrupts.
	OTG_FS_DIEPMSK = 0;
	OTG_FS_DOEPMSK = 0;
	OTG_FS_DAINTMSK = 0;
	OTG_FS_DIEPEMPMSK = 0;
	OTG_FS_DOEPINT0 = B2BSTUP | OTEPDIS | STUP | EPDISD | XFRC;
	OTG_FS_DOEPINT1 = B2BSTUP | OTEPDIS | STUP | EPDISD | XFRC;
	OTG_FS_DOEPINT2 = B2BSTUP | OTEPDIS | STUP | EPDISD | XFRC;
	OTG_FS_DOEPINT3 = B2BSTUP | OTEPDIS | STUP | EPDISD | XFRC;
	OTG_FS_GINTMSK = USBRSTM | ENUMDNEM | GONAKEFFM | GINAKEFFM | RXFLVLM | OTGINT;
	OTG_FS_GINTSTS = OTG_FS_GINTSTS & ~ENUMDNE;
}

static void handle_reset(void) {
	// Handle the reset under global NAK.
	static usb_ll_gnak_req_t req = USB_LL_GNAK_REQ_INIT;
	usb_ll_set_gnak(&req, &handle_reset_gnak);
}

static void handle_enumeration_done(void) {
	// Update state.
	device_state = USB_LL_STATE_ACTIVE;

	// Enable interrupts on IN transfer complete and receive FIFO non-empty.
	OTG_FS_DIEPMSK |= XFRCM;
	OTG_FS_GINTMSK |= GINTMSK_IEPINT;

	// Disable enumeration complete interrupt; this shouldn’t happen any more.
	OTG_FS_GINTMSK &= ~ENUMDNEM;

	// Notify the application.
	if (enumeration_done_cb) {
		enumeration_done_cb();
	}
}

usb_ll_state_t usb_ll_get_state(void) {
	return device_state;
}

void usb_ll_attach(usb_ll_reset_cb_t the_reset_cb, usb_ll_enumeration_done_cb_t the_enumeration_done_cb, usb_ll_unplug_cb_t the_unplug_cb) {
	// Sanity check.
	assert(device_state == USB_LL_STATE_DETACHED);

	// Initialize variables.
	device_state = USB_LL_STATE_POWERED;
	reset_cb = the_reset_cb;
	enumeration_done_cb = the_enumeration_done_cb;
	unplug_cb = the_unplug_cb;
	for (unsigned int i = 0; i < 4; ++i) {
		in_endpoint_callbacks[i] = 0;
		out_endpoint_callbacks[i] = 0;
	}
	for (usb_ll_gnak_req_t *req = gnak_requests_head; req; req = req->next) {
		req->queued = false;
	}
	gnak_requests_head = gnak_requests_tail = 0;
	gonak_requested = false;
	ginak_requested = false;

	// Reset the module and enable the clock
	rcc_enable(AHB2, 7);

	// Reset the USB core and configure device-wide parameters
	OTG_FS_GUSBCFG =
		0 // CTXPKT = 0; this bit should never be set
		| FDMOD // Force to device mode (no cable ID used)
		| 0 // FHMOD = 0; do not force to host mode
		| TRDT(5) // Turnaround time 5 PHY clocks to synchronize (there is a formula in the datasheet, but the STM32 library ignores it and just uses 5)
		| 0 // HNPCAP = 0; not host-negotiation-protocol capable
		| 0 // SRPCAP = 0; not session-request-protocol capable
		| PHYSEL // This bit is always set
		| TOCAL(0); // Do not add additional bit times to interpacket timeout (the STMicro library leaves this at zero; I assume this is fine for the on-chip PHY)
	while (!(OTG_FS_GRSTCTL & AHBIDL)); // Wait until AHB is idle
	OTG_FS_GRSTCTL |= CSRST; // CSRST = 1; core soft reset
	while (OTG_FS_GRSTCTL & CSRST); // Wait for reset to be complete
	// Wait at least 3 PHY clocks (would be 62.5 ns)
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	while (!(OTG_FS_GRSTCTL & AHBIDL)); // Wait until AHB is idle
	OTG_FS_GCCFG =
		NOVBUSSENS // VBUS sensing is not done
		| 0 // SOFOUTEN = 0; do not output SOF pulses to I/O pin
		| VBUSBSEN // VBUS sensing B device enabled
		| 0 // VBUSASEN = 0; VBUS sensing A device disabled
		| PWRDWN; // Transceiver active
	OTG_FS_GAHBCFG =
		0 // PTXFELVL = 0; only used in host mode
		| 0 // TXFELVL = 0; interrupt on TX FIFO half empty
		| 0; // GINTMSK = 0; no interrupts
	sleep_ms(25); // The application must wait at least 25 ms before a change to FDMOD takes effect
	OTG_FS_DCFG =
		0 // PFIVL = 0; end of periodic frame notification occurs at 80% of complete frame
		| 0 // DAD = 0; device does not yet have an address
		| NZLSOHSK // Send STALL on receipt of non-zero-length status transaction
		| DSPD(3); // Run at full speed
	OTG_FS_GOTGCTL =
		0 // DHNPEN = 0; host negotiation protocol disabled
		| 0 // HSHNPEN = 0; host negotiation protocol has not been enabled on the peer
		| 0 // HNPRQ = 0; do not issue host negotiation protocol request
		| 0; // SRQ = 0; do not issue session request
	OTG_FS_DCTL =
		CGONAK // CGONAK = 1; do not do OUT NAKs
		| CGINAK; // CGINAK = 1; do not do IN NAKs

	// Clear pending OTG interrupts.
	OTG_FS_GOTGINT = ADTOCHG | HNGDET | HNSSCHG | SRSSCHG | SEDET;

	// Enable interrupts on USB reset, bus unplug, and RX FIFO activity.
	OTG_FS_GINTMSK = USBRSTM | GONAKEFFM | GINAKEFFM | RXFLVLM | OTGINT;
	OTG_FS_GAHBCFG |= GINTMSK;
}

void usb_ll_detach(void) {
	OTG_FS_DCTL |= SDIS; // Soft disconnect from bus
	OTG_FS_GAHBCFG &= ~GINTMSK; // Disable USB interrupts globally
	OTG_FS_GCCFG &= ~PWRDWN; // Transceiver inactive
	rcc_disable(AHB2, 7); // Power down the module
	device_state = USB_LL_STATE_DETACHED;
}



static void handle_in_endpoint(void) {
	// Find out which endpoint had the interrupt and dispatch to its registered callback.
	uint32_t daint = OTG_FS_DAINT;
	for (unsigned int i = 0; i <= 3; ++i) {
		if (DAINT_IEPINT_X(daint) & (1 << i)) {
			in_endpoint_callbacks[i](i);
			return;
		}
	}
}

void usb_ll_in_set_cb(unsigned int ep, usb_ll_in_cb_t cb) {
	assert(ep <= 3);
	in_endpoint_callbacks[ep] = cb;
}

static void handle_receive_fifo_nonempty(void) {
	// Pop a status word from the FIFO.
	uint32_t status_word = OTG_FS_GRXSTSP & (FRMNUM_MSK | PKTSTS_MSK | GRXSTSP_DPID_MSK | BCNT_MSK | GRXSTSP_EPNUM_MSK);

	if (PKTSTS_X(status_word) == 0x1) {
		// This is a notification of global OUT NAK effectiveness.
		// We actually don’t do anything here, because the engine automatically sets an interrupt flag when we pop this pattern.
		// We will handle the situation through that interrupt.
		// The pattern just serves to serialize the OUT NAK effectiveness with received data packets.
	} else {
		// This is an endpoint-specific pattern.
		// Deliver it to the callback for the endpoint.
		unsigned int endpoint = GRXSTSP_EPNUM_X(status_word);
		if (out_endpoint_callbacks[endpoint]) {
			out_endpoint_callbacks[endpoint](endpoint, status_word);
		}
	}
}

void usb_ll_out_set_cb(unsigned int ep, usb_ll_out_cb_t cb) {
	assert(ep <= 3);
	out_endpoint_callbacks[ep] = cb;
}



static void handle_gnak_effective(void) {
	// We need a single, atomic read of this register or else our logic will get confused.
	uint32_t gintsts = OTG_FS_GINTSTS;

	if (gnak_requests_head) {
		// There are queued requests for global NAK.
		if ((gintsts & GOUTNAKEFF) && (gintsts & GINAKEFF)) {
			// Both IN and OUT NAK are effective.
			gonak_requested = false;
			ginak_requested = false;
			// Run the global NAK request callbacks now.
			// This is a careful traversal that handles concurrent list modification.
			while (gnak_requests_head) {
				usb_ll_gnak_req_t *req = gnak_requests_head;
				gnak_requests_head = req->next;
				if (!gnak_requests_head) {
					gnak_requests_tail = 0;
				}
				req->queued = false;
				if (req->cb) {
					req->cb();
				}
			}
			// We no longer have any requests, so disable global NAK.
			OTG_FS_DCTL |= CGONAK | CGINAK;
			// We want to be notified of any other global NAK effectivenesses occurring later.
			OTG_FS_GINTMSK |= GONAKEFFM | GINAKEFFM;
		} else {
			// Global NAK has been requested, but not all global NAKs have become effective yet.
			// We must disable the interrupt for the specific direction that has become effective (to avoid endless interrupts).
			// Then we must return and wait for the other direction that is not yet effective to become effective.
			if (gintsts & GOUTNAKEFF) {
				gonak_requested = false;
				OTG_FS_GINTMSK &= ~GONAKEFFM;
			}
			if (gintsts & GINAKEFF) {
				ginak_requested = false;
				OTG_FS_GINTMSK &= ~GINAKEFFM;
			}
		}
	} else {
		// There are no queued requests for global NAK.
		// This is spurious, perhaps caused by an old request.
		// We should cancel whatever global NAK is effective.
		if (gintsts & GOUTNAKEFF) {
			gonak_requested = false;
			OTG_FS_DCTL |= CGONAK;
		}
		if (gintsts & GINAKEFF) {
			ginak_requested = false;
			OTG_FS_DCTL |= CGINAK;
		}
		// We want to be notified of any other global NAK effectivenesses occurring later.
		OTG_FS_GINTMSK |= GONAKEFFM | GINAKEFFM;
	}
}

void usb_ll_set_gnak(usb_ll_gnak_req_t *req, usb_ll_gnak_cb_t cb) {
	req->cb = cb;

	if (!req->queued) {
		// This request is not already registered.
		// We must do something interesting.
		// Push the request into the list.
		req->queued = true;
		req->next = 0;
		if (gnak_requests_tail) {
			gnak_requests_tail->next = req;
			gnak_requests_tail = req;
		} else {
			gnak_requests_head = gnak_requests_tail = req;
		}
		// If any direction’s global NAK is neither effective nor requested, request it now.
		if (!gonak_requested && !(OTG_FS_GINTSTS & GOUTNAKEFF)) {
			OTG_FS_DCTL |= SGONAK;
			gonak_requested = true;
		}
		if (!ginak_requested && !(OTG_FS_GINTSTS & GINAKEFF)) {
			OTG_FS_DCTL |= SGINAK;
			ginak_requested = true;
		}
	}
}



static void handle_otg_interrupt(void) {
	uint32_t otgint = OTG_FS_GOTGINT;
	OTG_FS_GOTGINT = otgint;
	if (otgint & SEDET) {
		OTG_FS_GOTGINT = SEDET;
		if (unplug_cb) {
			unplug_cb();
		}
	}
}

void usb_ll_process(void) {
	uint32_t mask = OTG_FS_GINTSTS & OTG_FS_GINTMSK;
	if (mask & USBRST) {
		OTG_FS_GINTSTS = USBRST;
		handle_reset();
	} else if (mask & ENUMDNE) {
		OTG_FS_GINTSTS = ENUMDNE;
		handle_enumeration_done();
	} else if (mask & RXFLVL) {
		// This interrupt is level-sensitive; it clears automatically when the FIFO is read.
		handle_receive_fifo_nonempty();
	} else if (mask & GINTSTS_IEPINT) {
		// This interrupt is level-sensitive; it clears automatically when the more specific interrupt bit is cleared.
		handle_in_endpoint();
	} else if ((mask & GINAKEFF) || (mask & GOUTNAKEFF)) {
		// These interrupts are level-sensitive; the handler will disable the mask if they need to be ignored for a while.
		handle_gnak_effective();
	} else if (mask & OTGINT) {
		// This interrupt is level-sensitive; it clears automatically when the more specific interrupt bit is cleared.
		handle_otg_interrupt();
	}
}

