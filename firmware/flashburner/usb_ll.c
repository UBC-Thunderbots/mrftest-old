#include "usb_ll.h"
#include <assert.h>
#include <rcc.h>
#include <registers/otg_fs.h>
#include <sleep.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static volatile usb_ll_state_t device_state = USB_LL_STATE_DETACHED;
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

	// Enable enumeration complete and reset interrupts only, and clear any pending interrupts (except enumeration complete itself, which might have happened in the intervening time).
	{
		OTG_FS_DIEPMSK_t tmp = { 0 };
		OTG_FS_DIEPMSK = tmp;
	}
	{
		OTG_FS_DOEPMSK_t tmp = { 0 };
		OTG_FS_DOEPMSK = tmp;
	}
	{
		OTG_FS_DAINTMSK_t tmp = { 0 };
		OTG_FS_DAINTMSK = tmp;
	}
	{
		OTG_FS_DIEPEMPMSK_t tmp = { 0 };
		OTG_FS_DIEPEMPMSK = tmp;
	}
	for (unsigned int i = 0; i <= 3; ++i) {
		OTG_FS_DOEPINTx_t tmp = {
			.B2BSTUP = 1,
			.OTEPDIS = 1,
			.STUP = 1,
			.EPDISD = 1,
			.XFRC = 1,
		};
		OTG_FS_DOEP[i].DOEPINT = tmp;
	}
	{
		OTG_FS_GINTMSK_t tmp = {
			.USBRST = 1,
			.ENUMDNEM = 1,
			.GONAKEFFM = 1,
			.GINAKEFFM = 1,
			.RXFLVLM = 1,
			.OTGINT = 1,
		};
		OTG_FS_GINTMSK = tmp;
	}
	{
		OTG_FS_GINTSTS_t tmp = OTG_FS_GINTSTS;
		tmp.ENUMDNE = 0;
		OTG_FS_GINTSTS = tmp;
	}
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
	// Disable enumeration complete interrupt; this shouldn’t happen any more.
	OTG_FS_DIEPMSK.XFRCM = 1;
	{
		OTG_FS_GINTMSK_t tmp = OTG_FS_GINTMSK;
		tmp.IEPINT = 1;
		tmp.ENUMDNEM = 0;
		OTG_FS_GINTMSK = tmp;
	}

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
	rcc_enable(AHB2, OTGFS);
	rcc_reset(AHB2, OTGFS);
	// Reset the USB core and configure device-wide parameters
	{
		OTG_FS_GUSBCFG_t tmp = {
			.CTXPKT = 0, // This bit should never be set
			.FDMOD = 1, // Force to device mode (no cable ID used)
			.FHMOD = 0, // Do not force to host mode
			.TRDT = 5, // Turnaround time 5 PHY clocks to synchronize (there is a formula in the datasheet, but the STM32 library ignores it and just uses 5)
			.HNPCAP = 0, // Not host-negotiation-protocol capable
			.SRPCAP = 0, // Not session-request-protocol capable
			.PHYSEL = 1, // This bit is always set
			.TOCAL = 0, // Do not add additional bit times to interpacket timeout (the STMicro library leaves this at zero; I assume this is fine for the on-chip PHY)
		};
		OTG_FS_GUSBCFG = tmp;
	}
	while (!OTG_FS_GRSTCTL.AHBIDL); // Wait until AHB is idle
	OTG_FS_GRSTCTL.CSRST = 1; // Core soft reset
	while (OTG_FS_GRSTCTL.CSRST); // Wait for reset to be complete
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
	while (!OTG_FS_GRSTCTL.AHBIDL); // Wait until AHB is idle
	{
		OTG_FS_GCCFG_t tmp = {
			.NOVBUSSENS = 1, // VBUS sensing is not done
			.SOFOUTEN = 0, // Do not output SOF pulses to I/O pin
			.VBUSBSEN = 1, // VBUS sensing B device enabled
			.VBUSASEN = 0, // VBUS sensing A device disabled
			.PWRDWN = 1, // Transceiver active
		};
		OTG_FS_GCCFG = tmp;
	}
	{
		OTG_FS_GAHBCFG_t tmp = {
			.PTXFELVL = 0, // Only used in host mode
			.TXFELVL = 0, // Interrupt on TX FIFO half empty
			.GINTMSK = 0, // No interrupts
		};
		OTG_FS_GAHBCFG = tmp;
	}
	sleep_ms(25); // The application must wait at least 25 ms before a change to FDMOD takes effect
	{
		OTG_FS_DCFG_t tmp = {
			.PFIVL = 0, // End of periodic frame notification occurs at 80% of complete frame
			.DAD = 0, // Device does not yet have an address
			.NZLSOHSK = 1, // Send STALL on receipt of non-zero-length status transaction
			.DSPD = 3, // Run at full speed
		};
		OTG_FS_DCFG = tmp;
	}
	{
		OTG_FS_GOTGCTL_t tmp = {
			.DHNPEN = 0, // Host negotiation protocol disabled
			.HSHNPEN = 0, // Host negotiation protocol has not been enabled on the peer
			.HNPRQ = 0, // Do not issue host negotiation protocol request
			.SRQ = 0, // Do not issue session request
		};
		OTG_FS_GOTGCTL = tmp;
	}
	{
		OTG_FS_DCTL_t tmp = {
			.POPRGDNE = 0, // Not waking from power-down mode right now
			.CGONAK = 1, // Do not do OUT NAKs
			.SGONAK = 0, // Do not do OUT NAKs
			.CGINAK = 1, // Do not do IN NAKs
			.SGINAK = 0, // Do not do IN NAKs
			.TCTL = 0, // Do not enable any test modes
			.SDIS = 0, // Do not disconnect from bus
			.RWUSIG = 0, // Do not generate remote wakeup signalling
		};
		OTG_FS_DCTL = tmp;
	}

	// Clear pending OTG interrupts.
	{
		OTG_FS_GOTGINT_t tmp = {
			.ADTOCHG = 1,
			.HNGDET = 1,
			.HNSSCHG = 1,
			.SRSSCHG = 1,
			.SEDET = 1,
		};
		OTG_FS_GOTGINT = tmp;
	}

	// Enable interrupts on USB reset, global NAK effective, bus unplug, and RX FIFO activity.
	{
		OTG_FS_GINTMSK_t tmp = {
			.USBRST = 1,
			.GONAKEFFM = 1,
			.GINAKEFFM = 1,
			.RXFLVLM = 1,
			.OTGINT = 1,
		};
		OTG_FS_GINTMSK = tmp;
	}
	OTG_FS_GAHBCFG.GINTMSK = 1;
}

void usb_ll_detach(void) {
	OTG_FS_DCTL.SDIS = 1; // Soft disconnect from bus
	OTG_FS_GAHBCFG.GINTMSK = 0; // Disable USB interrupts globally
	OTG_FS_GCCFG.PWRDWN = 0; // Transceiver inactive
	rcc_disable(AHB2, OTGFS); // Power down the module
	device_state = USB_LL_STATE_DETACHED;
}



static void handle_in_endpoint(void) {
	// Find out which endpoint had the interrupt and dispatch to its registered callback.
	unsigned int mask = OTG_FS_DAINT.IEPINT;
	for (unsigned int i = 0; i <= 3; ++i) {
		if (mask & (1 << i)) {
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
	OTG_FS_GRXSTSR_device_t status_word = OTG_FS_GRXSTSP.device;

	if (status_word.PKTSTS == 0x1) {
		// This is a notification of global OUT NAK effectiveness.
		// We actually don’t do anything here, because the engine automatically sets an interrupt flag when we pop this pattern.
		// We will handle the situation through that interrupt.
		// The pattern just serves to serialize the OUT NAK effectiveness with received data packets.
	} else {
		// This is an endpoint-specific pattern.
		// Deliver it to the callback for the endpoint.
		if (out_endpoint_callbacks[status_word.EPNUM]) {
			out_endpoint_callbacks[status_word.EPNUM](status_word.EPNUM, status_word);
		}
	}
}

void usb_ll_out_set_cb(unsigned int ep, usb_ll_out_cb_t cb) {
	assert(ep <= 3);
	out_endpoint_callbacks[ep] = cb;
}



static void handle_gnak_effective(void) {
	// We need a single, atomic read of this register or else our logic will get confused.
	OTG_FS_GINTSTS_t gintsts = OTG_FS_GINTSTS;

	if (gnak_requests_head) {
		// There are queued requests for global NAK.
		if ((gintsts.GOUTNAKEFF) && (gintsts.GINAKEFF)) {
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
			{
				OTG_FS_DCTL_t tmp = OTG_FS_DCTL;
				tmp.CGONAK = 1;
				tmp.CGINAK = 1;
				OTG_FS_DCTL = tmp;
			}
			// We want to be notified of any other global NAK effectivenesses occurring later.
			{
				OTG_FS_GINTMSK_t tmp = OTG_FS_GINTMSK;
				tmp.GONAKEFFM = 1;
				tmp.GINAKEFFM = 1;
				OTG_FS_GINTMSK = tmp;
			}
		} else {
			// Global NAK has been requested, but not all global NAKs have become effective yet.
			// We must disable the interrupt for the specific direction that has become effective (to avoid endless interrupts).
			// Then we must return and wait for the other direction that is not yet effective to become effective.
			if (gintsts.GOUTNAKEFF) {
				gonak_requested = false;
				OTG_FS_GINTMSK.GONAKEFFM = 0;
			}
			if (gintsts.GINAKEFF) {
				ginak_requested = false;
				OTG_FS_GINTMSK.GINAKEFFM = 0;
			}
		}
	} else {
		// There are no queued requests for global NAK.
		// This is spurious, perhaps caused by an old request.
		// We should cancel whatever global NAK is effective.
		if (gintsts.GOUTNAKEFF) {
			gonak_requested = false;
			OTG_FS_DCTL.CGONAK = 1;
		}
		if (gintsts.GINAKEFF) {
			ginak_requested = false;
			OTG_FS_DCTL.CGINAK = 1;
		}
		// We want to be notified of any other global NAK effectivenesses occurring later.
		{
			OTG_FS_GINTMSK_t tmp = OTG_FS_GINTMSK;
			tmp.GONAKEFFM = 1;
			tmp.GINAKEFFM = 1;
			OTG_FS_GINTMSK = tmp;
		}
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
		if (!gonak_requested && !OTG_FS_GINTSTS.GOUTNAKEFF) {
			OTG_FS_DCTL.SGONAK = 1;
			gonak_requested = true;
		}
		if (!ginak_requested && !OTG_FS_GINTSTS.GINAKEFF) {
			OTG_FS_DCTL.SGINAK = 1;
			ginak_requested = true;
		}
	}
}



static void handle_otg_interrupt(void) {
	OTG_FS_GOTGINT_t otgint = OTG_FS_GOTGINT;
	OTG_FS_GOTGINT = otgint;
	if (otgint.SEDET) {
		if (unplug_cb) {
			unplug_cb();
		}
	}
}

void usb_ll_process(void) {
	// We rely here on the fact that GINTSTS and GINTMSK have parallel layouts, so we can AND their bits together to get something useful.
	// This gives us only those interrupts that are both true and interesting.
	OTG_FS_GINTSTS_t mask;
	{
		OTG_FS_GINTSTS_t gintsts = OTG_FS_GINTSTS;
		OTG_FS_GINTMSK_t gintmsk = OTG_FS_GINTMSK;
		uint32_t gintsts32, gintmsk32;
		memcpy(&gintsts32, &gintsts, sizeof(uint32_t));
		memcpy(&gintmsk32, &gintmsk, sizeof(uint32_t));
		uint32_t final = gintsts32 & gintmsk32;
		memcpy(&mask, &final, sizeof(uint32_t));
	}
	if (mask.USBRST) {
		{
			OTG_FS_GINTSTS_t tmp = { .USBRST = 1 };
			OTG_FS_GINTSTS = tmp;
		}
		handle_reset();
	} else if (mask.ENUMDNE) {
		{
			OTG_FS_GINTSTS_t tmp = { .ENUMDNE = 1 };
			OTG_FS_GINTSTS = tmp;
		}
		handle_enumeration_done();
	} else if (mask.RXFLVL) {
		// This interrupt is level-sensitive; it clears automatically when the FIFO is read.
		handle_receive_fifo_nonempty();
	} else if (mask.IEPINT) {
		// This interrupt is level-sensitive; it clears automatically when the more specific interrupt bit is cleared.
		handle_in_endpoint();
	} else if (mask.GINAKEFF || mask.GOUTNAKEFF) {
		// These interrupts are level-sensitive; the handler will disable the mask if they need to be ignored for a while.
		handle_gnak_effective();
	} else if (mask.OTGINT) {
		// This interrupt is level-sensitive; it clears automatically when the more specific interrupt bit is cleared.
		handle_otg_interrupt();
	}
}

