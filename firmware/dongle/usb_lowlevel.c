#include "error_reporting.h"
#include "usb.h"
#include "usb_internal.h"
#include <pic18fregs.h>
#include <stdint.h>
#include <string.h>

/**
 * \file
 *
 * \brief Implements the low-level packet handling for the USB subsystem.
 */

usb_epn_callbacks_t usb_ep_callbacks[USB_CONFIG_MAX_ENDPOINT + 1];

#if USB_CONFIG_SOF_CALLBACK
void (*usb_sof_callback)(void);
#endif

#if !USB_CONFIG_IDLE
static
#endif
volatile BOOL usb_is_idle;

/**
 * \brief The value of the UIE register saved when the bus becomes idle.
 */
static uint8_t idle_saved_uie;

void usb_lowlevel_init(void) {
	PIE2bits.USBIE = 0;
	UCON = 0;

	UCFGbits.UTEYE = 0;
	UCFGbits.UOEMON = 0;
	UCFGbits.UPUEN = USB_CONFIG_ONBOARD_PULLUPS;
	UCFGbits.UTRDIS = USB_CONFIG_EXTERNAL_TRANSCEIVER;
	UCFGbits.FSEN = USB_CONFIG_FULL_SPEED;
	UCFGbits.PPB0 = 0;
	UCFGbits.PPB1 = 0;

	/* Set EPCONDIS, clear other bits. */
	UEP0 = 0x08;
	UEP1 = 0x08;
	UEP2 = 0x08;
	UEP3 = 0x08;
	UEP4 = 0x08;
	UEP5 = 0x08;
	UEP6 = 0x08;
	UEP7 = 0x08;
	UEP8 = 0x08;
	UEP9 = 0x08;
	UEP10 = 0x08;
	UEP11 = 0x08;
	UEP12 = 0x08;
	UEP13 = 0x08;
	UEP14 = 0x08;
	UEP15 = 0x08;

	UEIR = 0;
	UEIE = 0b10011111;

	UIE = 0;
#if USB_CONFIG_SOF_CALLBACK
	UIEbits.SOFIE = 1;
#endif
	UIEbits.IDLEIE = USB_CONFIG_IDLE;
	UIEbits.ACTVIE = 1;
	UIEbits.URSTIE = 1;
	UIEbits.UERRIE = 1;
	UIEbits.STALLIE = 1;

	UCONbits.USBEN = 1;

	while (UCONbits.SE0);

	while (UIR) {
		UIR = 0;
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
	}

#if USB_CONFIG_SOF_CALLBACK
	usb_sof_callback = 0;
#endif

	usb_is_idle = false;

	PIR2bits.USBIF = 0;
	IPR2bits.USBIP = USB_CONFIG_INTERRUPTS_HIGH_PRIORITY;
	PIE2bits.USBIE = 1;
}

void usb_lowlevel_deinit(void) {
	PIE2bits.USBIE = 0;
	UCONbits.USBEN = 0;
}

SIGHANDLER(usb_process) {
	if (usb_is_idle) {
		if (UIRbits.ACTVIF) {
			UCONbits.SUSPND = 0;
			usb_is_idle = false;
			UIE = idle_saved_uie;
			while (UIRbits.ACTVIF) {
				UIRbits.ACTVIF = 0;
			}
		}
	} else {
		if (UIRbits.IDLEIF) {
			UCONbits.SUSPND = 1;
			usb_is_idle = true;
			idle_saved_uie = UIE;
			UIE = 0;
			UIEbits.ACTVIE = 1;
			UIRbits.IDLEIF = 0;
		} else
		if (UIRbits.URSTIF) {
			while (UIRbits.TRNIF) {
				UIRbits.TRNIF = 0;
				Nop();
				Nop();
				Nop();
				Nop();
				Nop();
				Nop();
			}
			memset(usb_ep_callbacks, 0, sizeof(usb_ep_callbacks));
			usb_ep0_init();
			UIEbits.TRNIE = 1;
			UCONbits.PKTDIS = 0;
			UIRbits.URSTIF = 0;
#if USB_CONFIG_SOF_CALLBACK
		} else if (UIRbits.SOFIF) {
			if (usb_sof_callback) {
				usb_sof_callback();
			}
			UIRbits.SOFIF = 0;
#endif
		} else if (UIRbits.TRNIF && UIEbits.TRNIE) {
			while (UIRbits.TRNIF) {
				uint8_t ep = (USTATbits.ENDP3 << 3) | (USTATbits.ENDP2 << 2) | (USTATbits.ENDP1 << 1) | USTATbits.ENDP0;
				if (USTATbits.DIR) {
					usb_ep_callbacks[ep].in.transaction();
				} else {
					usb_ep_callbacks[ep].out.transaction();
				}
				UIRbits.TRNIF = 0;
				Nop();
				Nop();
				Nop();
				Nop();
				Nop();
				Nop();
				Nop();
			}
		} else if (UIRbits.UERRIF) {
			if (UEIRbits.BTSEF) {
				error_reporting_add(FAULT_BTSEF);
			}
			if (UEIRbits.BTOEF) {
				error_reporting_add(FAULT_BTOEF);
			}
			if (UEIRbits.DFN8EF) {
				error_reporting_add(FAULT_DFN8EF);
			}
			if (UEIRbits.CRC16EF) {
				error_reporting_add(FAULT_CRC16EF);
			}
			if (UEIRbits.CRC5EF) {
				error_reporting_add(FAULT_CRC5EF);
			}
			if (UEIRbits.PIDEF) {
				error_reporting_add(FAULT_PIDEF);
			}
			UEIR = 0;
		} else if (UIRbits.STALLIF) {
			error_reporting_add(FAULT_STALL);
			UIRbits.STALLIF = 0;
		}
	}

	PIR2bits.USBIF = 0;
}

