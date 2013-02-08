#include "usb.h"
#include "assert.h"
#include "usb_fifo.h"
#include "usb_internal.h"
#include "rcc.h"
#include "registers.h"
#include "sleep.h"
#include "stdbool.h"

typedef enum {
	DEVICE_STATE_DETACHED,
	DEVICE_STATE_POWERED,
	DEVICE_STATE_ENUMERATING,
	DEVICE_STATE_ACTIVE,
	DEVICE_STATE_DETACHING,
} device_state_t;

static device_state_t device_state = DEVICE_STATE_DETACHED;
static usb_gnak_request_t *gnak_requests_head = 0, *gnak_requests_tail = 0;
static bool gonak_requested = false, ginak_requested = false;
const usb_device_info_t *usb_device_info = 0;
static usb_in_callback_t in_endpoint_callbacks[4] = { 0, 0, 0, 0 };
static usb_out_callback_t out_endpoint_callbacks[4] = { 0, 0, 0, 0 };

static void handle_reset_gnak(void);

static void handle_reset(void) {
	// Get global NAK going.
	// We will handle the rest of the initialization under global NAK.
	// This is necessary because, if the device was attached at the time, we might potentially need to call usb_ep0_deinit().
	// That function must only be called under global NAK.
	static usb_gnak_request_t req = USB_GNAK_REQUEST_INIT;
	usb_set_global_nak(&req, &handle_reset_gnak);
}

static void handle_reset_gnak(void) {
	// Sanity check.
	assert(device_state != DEVICE_STATE_DETACHED);

	// It’s possible a reset could have happened while the device was already connected. Handle it.
	if (device_state == DEVICE_STATE_ACTIVE) {
		usb_ep0_deinit();
		OTG_FS_GINTMSK = USBRSTM | GONAKEFFM | GINAKEFFM | RXFLVLM | OTGINT;
		OTG_FS_GINTSTS = OTG_FS_GINTSTS & ~ENUMDNE;
	}

	// Update state.
	device_state = DEVICE_STATE_ENUMERATING;

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

	// Configure receive FIFO and endpoint 0 transmit FIFO sizes
	size_t ep0_tx_fifo_words = usb_device_info->ep0_max_packet / 4;
	usb_fifo_init(ep0_tx_fifo_words);

	// Flush all transmit FIFOs and the receive FIFO.
	usb_fifo_flush(16);
	usb_fifo_rx_flush();

	// Invoke any registered callback.
	if (usb_device_info->on_reset) {
		usb_device_info->on_reset();
	}
}

static void handle_enumeration_done(void) {
	// Update state.
	device_state = DEVICE_STATE_ACTIVE;

	// Enable interrupts on IN transfer complete and receive FIFO non-empty.
	OTG_FS_DIEPMSK |= XFRCM;
	OTG_FS_GINTMSK |= GINTMSK_IEPINT;

	// Disable enumeration complete interrupt; this shouldn’t happen any more.
	OTG_FS_GINTMSK &= ~ENUMDNEM;

	// Initialize the endpoint 0 layer
	usb_ep0_init();
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
				usb_gnak_request_t *req = gnak_requests_head;
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

void usb_set_global_nak(usb_gnak_request_t *req, void (*cb)(void)) {
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
		unsigned int endpoint = GRXSTSP_EPNUM_X(status_word);
		if (out_endpoint_callbacks[endpoint]) {
			out_endpoint_callbacks[endpoint](endpoint, status_word);
		}
	}
}

static void handle_in_endpoint(void) {
	// Find out which endpoint had the interrupt and dispatch
	uint32_t daint = OTG_FS_DAINT;
	for (unsigned int i = 0; i < 4; ++i) {
		if (DAINT_IEPINT_X(daint) & (1 << i)) {
			in_endpoint_callbacks[i](i);
			return;
		}
	}
}

void usb_copy_out_packet(void *target, size_t length) {
	length = (length + 3) / 4;
	volatile const uint32_t *src = OTG_FS_FIFO[0];
	if (target) {
		uint32_t *dest = target;
		while (length--) {
			*dest++ = *src++;
		}
	} else {
		while (length--) {
			(void) *src++;
		}
	}
}

static void handle_otg_interrupt(void) {
	uint32_t otgint = OTG_FS_GOTGINT;
	OTG_FS_GOTGINT = otgint;
	if (otgint & SEDET) {
		usb_detach();
	}
}

void usb_process(void) {
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

void usb_attach(const usb_device_info_t *info) {
	assert(device_state == DEVICE_STATE_DETACHED);

	// Initialize variables.
	device_state = DEVICE_STATE_POWERED;
	usb_device_info = info;
	gonak_requested = false;
	ginak_requested = false;
	for (usb_gnak_request_t *req = gnak_requests_head; req; req = req->next) {
		req->queued = false;
	}
	gnak_requests_head = gnak_requests_tail = 0;

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

	// Enable interrupts on USB reset, bus unplug, and RX FIFO activity.
	OTG_FS_GOTGINT = ADTOCHG | HNGDET | HNSSCHG | SRSSCHG | SEDET;
	OTG_FS_GINTMSK = USBRSTM | GONAKEFFM | GINAKEFFM | RXFLVLM | OTGINT;
	OTG_FS_GAHBCFG |= GINTMSK;
}

static void usb_detach_impl(void) {
	if (device_state == DEVICE_STATE_DETACHING) {
		usb_ep0_deinit();
	}

	OTG_FS_DCTL |= SDIS; // Soft disconnect from bus
	OTG_FS_GAHBCFG &= ~GINTMSK; // Disable USB interrupts globally
	OTG_FS_GCCFG &= ~PWRDWN; // Transceiver inactive
	rcc_disable(AHB2, 7); // Power down the module
	device_state = DEVICE_STATE_DETACHED;
}

void usb_detach(void) {
	if (device_state == DEVICE_STATE_ACTIVE) {
		static usb_gnak_request_t gnak_req = USB_GNAK_REQUEST_INIT;
		device_state = DEVICE_STATE_DETACHING;
		usb_set_global_nak(&gnak_req, &usb_detach_impl);
	} else {
		usb_detach_impl();
	}
}

void usb_in_set_callback(unsigned int ep, usb_in_callback_t cb) {
	in_endpoint_callbacks[ep] = cb;
}

void usb_out_set_callback(unsigned int ep, usb_out_callback_t cb) {
	out_endpoint_callbacks[ep] = cb;
}

