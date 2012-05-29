#include "usb.h"
#include "assert.h"
#include "usb_internal.h"
#include "registers.h"
#include "sleep.h"
#include "stdbool.h"

typedef enum {
	DEVICE_STATE_DETACHED,
	DEVICE_STATE_RESETTING,
	DEVICE_STATE_SPEED_ENUMERATING,
	DEVICE_STATE_CONNECTED,
} device_state_t;

static device_state_t device_state = DEVICE_STATE_DETACHED;
const usb_device_info_t *usb_device_info = 0;
static void (*(in_endpoint_callbacks[4]))(void) = { 0, 0, 0, 0 };
static bool global_nak_state = false;

static void handle_reset(void) {
	// Configure receive FIFO and endpoint 0 transmit FIFO sizes.
	size_t ep0_tx_fifo_words = usb_device_info->ep0_max_packet / 4;
	OTG_FS_GRXFSIZ =
		(OTG_FS_GRXFSIZ & 0xFFFF0000) // Reserved bits.
		| (usb_device_info->rx_fifo_words << 0); // RXFD; allocate this many words to receive FIFO.
	OTG_FS_DIEPTXF0 =
		(ep0_tx_fifo_words << 16) // TX0FD; allocate this many words to endpoint 0 transmit FIFO.
		| (usb_device_info->rx_fifo_words << 0); // TX0FSA; endpoint 0 transmit FIFO starts at this many words into FIFO RAM.
	while (!(OTG_FS_GRSTCTL & (1 << 31) /* AHBIDL */));
	OTG_FS_GRSTCTL =
		(16 << 6) // TXFNUM = b10000; flush all transmit FIFOs.
		| (1 << 5); // TXFFLSH = 1; flush transmit FIFOs.
	while (OTG_FS_GRSTCTL & (1 << 5) /* TXFFLSH */);
	OTG_FS_GRSTCTL = (1 << 4); // RXFFLSH = 1; flush receive FIFO.
	while (OTG_FS_GRSTCTL & (1 << 4) /* RXFFLSH */);
	OTG_FS_DOEPTSIZ0 =
		(OTG_FS_DOEPTSIZ0 & 0x9FF7FF80) // Reserved bits.
		| (3 << 29) // STUPCNT = 3; allow up to 3 back-to-back SETUP data packets.
		| (0 << 19) // PKTCNT = 0; no non-SETUP packets should be issued.
		| (0 << 0); // XFRSIZ = 0; no non-SETUP transfer is occurring.
	OTG_FS_GINTMSK |= 1 << 13; // ENUMDNEM = 1; take an interrupt when speed enumeration is complete.
}

static void handle_enumeration_done(void) {
	// Enable some interrupts.
	OTG_FS_DIEPMSK |= 1 << 0; // XFRCM = 1; take an interrupt on transfer complete.
	OTG_FS_GINTMSK |= 1 << 4; // RXFLVLM = 1; take an interrupt on RX FIFO non-empty

	// Initialize the endpoint 0 layer.
	usb_ep0_init();
}

static void handle_receive_fifo_nonempty(void) {
	// Pop a status word from the FIFO.
	uint32_t status_word = OTG_FS_GRXSTSP & ~0xFE000000;

	if ((status_word & (0xF << 17)) == (0x1 << 17)) {
		// This is a notification of global OUT NAK effectiveness
		if (global_nak_state) {
			OTG_FS_GINTMSK |= 1 << 6; // GINAKEFFM = 1; take an interrupt when global IN NAK becomes effective
			OTG_FS_DCTL = (OTG_FS_DCTL & ~(1 << 8) /* CGINAK = 0 */) | (1 << 7) /* SGINAK = 1 */;
		}
	} else {
		// This is an endpoint-specific pattern.
		uint8_t endpoint = status_word & 0x0F;
		if (!endpoint) {
			// This is a pattern to endpoint zero.
			usb_ep0_handle_receive(status_word);
		} else {
			// This is a pattern to a nonzero endpoint.
#warning TODO something sensible
		}
	}
}

static void handle_in_endpoint(void) {
	// Find out which endpoint had the interrupt and dispatch.
	uint32_t daint = OTG_FS_DAINT;
	for (unsigned int i = 0; i < 4; ++i) {
		if (daint & (1 << i)) {
			in_endpoint_callbacks[i]();
			return;
		}
	}
}

void usb_copy_out_packet(void *target, size_t length) {
	length = (length + 3) / 4;
	volatile const uint32_t *src = (volatile const uint32_t *) 0x50001000;
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

void usb_set_global_nak(void) {
	if (!global_nak_state) {
		global_nak_state = true;
		OTG_FS_DCTL |= 1 << 9; // SGONAK = 1; set global OUT NAK
	}
#if 0
	if (global_out_nak == GLOBAL_NAK_STATE_SET && global_in_nak == GLOBAL_NAK_STATE_SET) {
		usb_ep0_handle_global_nak_effective();
		return;
	}
	if (global_out_nak == GLOBAL_NAK_STATE_CLEAR) {
		OTG_FS_DCTL = (OTG_FS_DCTL & ~(1 << 10) /* CGONAK = 0 */) | (1 << 9) /* SGONAK = 1 */;
		global_out_nak = GLOBAL_NAK_STATE_PENDING;
	}
	if (global_in_nak == GLOBAL_NAK_STATE_CLEAR) {
		OTG_FS_DCTL = (OTG_FS_DCTL & ~(1 << 8) /* CGINAK = 0 */) | (1 << 7) /* SGINAK = 1 */;
		OTG_FS_GINTMSK |= 1 << 6; // GINAKEFFM = 1; take an interrupt when global IN NAK becomes effective
		global_in_nak = GLOBAL_NAK_STATE_PENDING;
	}
#endif
}

void usb_clear_global_nak(void) {
	OTG_FS_DCTL = (OTG_FS_DCTL & ~(1 << 9) /* SGONAK = 0 */ & ~(1 << 7) /* SGINAK = 0 */) | (1 << 10) /* CGONAK = 1 */ | (1 << 8) /* CGINAK = 1 */;
	OTG_FS_GINTMSK &= ~(1 << 6); // GINAKEFFM = 0; do not take an interrupt when global IN NAK becomes effective
	global_nak_state = false;
#if 0
	global_out_nak = GLOBAL_NAK_STATE_CLEAR;
	global_in_nak = GLOBAL_NAK_STATE_CLEAR;
#endif
}

void usb_process(void) {
	uint32_t mask = OTG_FS_GINTSTS & OTG_FS_GINTMSK;
	if (mask & (1 << 12) /* USBRST */) {
		OTG_FS_GINTSTS |= 1 << 12;
		handle_reset();
	} else if (mask & (1 << 13) /* ENUMDNE */) {
		OTG_FS_GINTSTS |= 1 << 13;
		handle_enumeration_done();
	} else if (mask & (1 << 4) /* RXFLVL */) {
		handle_receive_fifo_nonempty();
	} else if (mask & (1 << 18) /* IEPINT */) {
		handle_in_endpoint();
	} else if (mask & (1 << 19) /* OEPINT */) {
#warning TODO something sensible
	} else if (mask & (1 << 6) /* GINAKEFF */) {
		OTG_FS_GINTMSK &= ~(1 << 6); // GINAKEFFM = 0; do not take an interrupt when global IN NAK becomes effective
		usb_ep0_handle_global_nak_effective();
	}
}

void usb_attach(const usb_device_info_t *info) {
	device_state = DEVICE_STATE_RESETTING;
	usb_device_info = info;

	// Enable the clock.
	RCC_AHB2ENR |= (1 << 7); // OTGFSEN = 1; enable clock to USB FS.

	// Reset the entire module.
	RCC_AHB2RSTR |= 1 << 7; // OTGFSRST = 1; reset USB FS.
	RCC_AHB2RSTR &= ~(1 << 7); // OTGFSRST = 0; stop resetting USB FS.

	// Reset the USB core and configure device-wide parameters.
	OTG_FS_GUSBCFG =
		(OTG_FS_GUSBCFG & 0x1FFFC078) // Reserved bits.
		| (0 << 31) // CTXPKT = 0; this bit should never be set.
		| (1 << 30) // FDMOD = 1; force to device mode (no cable ID used).
		| (0 << 29) // FHMOD = 0; do not force to host mode.
		| (5 << 10) // TRDT = 5; turnaround time 5 PHY clocks to synchronize (there is a formula in the datasheet, but the STM32 library ignores it and just uses 5).
		| (0 << 9) // HNPCAP = 0; not host-negotiation-protocol capable.
		| (0 << 8) // SRPCAP = 0; not session-request-protocol capable.
		| (1 << 7) // PHYSEL = 1; this bit is always set.
		| (0 << 0); // TOCAL = 0; do not add additional bit times to interpacket timeout (the library leaves this at zero; I assume this is fine for the on-chip PHY).
	while (!(OTG_FS_GRSTCTL & (1 << 31) /* AHBIDL */)); // Wait until AHB is idle.
	OTG_FS_GRSTCTL |= (1 << 0); // CSRST = 1; core soft reset.
	while (OTG_FS_GRSTCTL & (1 << 0) /* CSRST */); // Wait for reset to be complete.
	for (unsigned int i = 0; i < 20; ++i) asm volatile("nop"); // Wait at least 3 PHY clocks (would be 62.5 ns).
	while (!(OTG_FS_GRSTCTL & (1 << 31) /* AHBIDL */)); // Wait until AHB is idle.
	OTG_FS_GCCFG =
		(OTG_FS_GCCFG & 0xFFC2FFFF)
		| (1 << 21) // NOVBUSSENS = 1; VBUS sensing is not done.
		| (0 << 20) // SOFOUTEN = 0; do not output SOF pulses to I/O pin.
		| (1 << 19) // VBUSBSEN = 1; VBUS sensing B device enabled.
		| (0 << 18) // VBUSASEN = 0; VBUS sensing A device disabled.
		| (1 << 16); // PWRDWN = 1; transceiver active.
	OTG_FS_GAHBCFG =
		(OTG_FS_GAHBCFG & 0xFFFFFE7E) // Reserved bits.
		| (0 << 8) // PTXFELVL = 0; only used in host mode.
		| (0 << 7) // TXFELVL = 0; interrupt on TX FIFO half empty.
		| (0 << 0); // GINTMSK = 0; no interrupts.
	sleep_millis(30); // The application must wait at least 25 ms before a change to FDMOD takes effect.
	OTG_FS_DCFG =
		(OTG_FS_DCFG & 0xFFFFE008) // Reserved bits.
		| (0 << 11) // PFIVL = 0; end of periodic frame notification occurs at 80% of complete frame.
		| (0 << 4) // DAD = 0; device does not yet have an address.
		| (1 << 2) // NZLSOHSK = 1; send STALL on receipt of non-zero-length status transaction.
		| (3 << 0); // DSPD = b11; run at full speed.
	OTG_FS_GOTGCTL =
		(OTG_FS_GOTGCTL & 0xFFF0F0FC) // Reserved bits.
		| (0 << 11) // DHNPEN = 0; host negotiation protocol disabled.
		| (0 << 10) // HSHNPEN = 0; host negotiation protocol has not been enabled on the peer.
		| (0 << 9) // HNPRQ = 0; do not issue host negotiation protocol request.
		| (0 << 1); // SRQ = 0; do not issue session request.

	// Enable an interrupt on USB reset.
	OTG_FS_GINTMSK = 1 << 12; // USBRST = 1; take an interrupt on USB reset.
	OTG_FS_GAHBCFG |= 1 << 0; // GINTMSK = 1; enable USB interrupts globally.
}

void usb_detach(void) {
	OTG_FS_DCTL |= 1 << 1; // SDIS = 1; soft disconnect from bus.
	OTG_FS_GAHBCFG &= ~(1 << 0); // GINTMSK = 0; disable USB interrupts globally.
}

void usb_in_set_callback(uint8_t ep, void (*cb)(void)) {
	in_endpoint_callbacks[ep] = cb;
}

