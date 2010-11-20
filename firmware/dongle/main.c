#include "descriptors.h"
#include "pins.h"
#include "signal.h"
#include "usb.h"
#include <delay.h>
#include <pic18fregs.h>
#include <string.h>

/**
 * \file
 *
 * \brief Contains the application entry point.
 */

SIGHANDLER(usb_handler) {
	usb_process();
}

DEF_INTHIGH(high_handler)
	DEF_HANDLER2(SIG_USB, SIG_USBIE, usb_handler)
END_DEF

static uint8_t intf0_buffer;
static uint8_t intf0_idle_rate;
static uint16_t intf0_last_tx_frame;
static BOOL intf0_halted;

static uint16_t button_change_frame;

static __code const void *find_interface_hid_class_descriptor(uint8_t interface) {
	__code const uint8_t *ptr = CONFIGURATION_DESCRIPTOR_TAIL;
	uint8_t current_interface = 0xFF;

	while (current_interface != interface) {
		if (ptr[1] == USB_DESCRIPTOR_INTERFACE) {
			++current_interface;
		}
		ptr += ptr[0];
	}

	while (ptr[1] != 0x21) {
		ptr += ptr[0];
	}

	return ptr;
}

static BOOL custom_setup_handler(void) {
	switch (usb_ep0_setup_buffer.request_type.bits.type) {
		case USB_SETUP_PACKET_REQUEST_STANDARD:
			if (usb_ep0_setup_buffer.request_type.bits.recipient == USB_SETUP_PACKET_RECIPIENT_INTERFACE) {
				if (usb_ep0_setup_buffer.request == USB_SETUP_PACKET_STDREQ_GET_DESCRIPTOR) {
					switch (usb_ep0_setup_buffer.value >> 8) {
						case 0x21: /* Class descriptor */
							if (usb_ep0_setup_buffer.index < 1) {
								usb_ep0_data[0].ptr = find_interface_hid_class_descriptor(usb_ep0_setup_buffer.index);
								usb_ep0_data[0].length = *((const uint8_t *) usb_ep0_data[0].ptr);
								usb_ep0_data_length = 1;
								return true;
							}
							break;

						case 0x22: /* Report descriptor */
							switch ((uint8_t) usb_ep0_setup_buffer.index) {
								case 0:
									usb_ep0_data[0].ptr = ESTOP_REPORT_DESCRIPTOR;
									usb_ep0_data[0].length = sizeof(ESTOP_REPORT_DESCRIPTOR);
									usb_ep0_data_length = 1;
									return true;
							}
							break;
					}
				}
			}
			break;

		case USB_SETUP_PACKET_REQUEST_CLASS:
			if (usb_ep0_setup_buffer.request_type.bits.recipient == USB_SETUP_PACKET_RECIPIENT_INTERFACE) {
				switch (usb_ep0_setup_buffer.request) {
					case 0x01: /* GET_REPORT */
						if (usb_ep0_setup_buffer.value >> 8 == 1) {
							switch (usb_ep0_setup_buffer.index) {
								case 0:
									usb_ep0_data[0].ptr = &intf0_buffer;
									usb_ep0_data[0].length = 1;
									usb_ep0_data_length = 1;
									return true;
							}
						}
						break;

					case 0x02: /* GET_IDLE */
						switch (usb_ep0_setup_buffer.index) {
							case 0:
								usb_ep0_data[0].ptr = &intf0_idle_rate;
								usb_ep0_data[0].length = 1;
								usb_ep0_data_length = 1;
								return true;
						}
						break;

					case 0x0A: /* SET_IDLE */
						switch (usb_ep0_setup_buffer.index) {
							case 0:
								intf0_idle_rate = usb_ep0_setup_buffer.value >> 8;
								return true;
						}
						break;
				}
			}
			break;
	}
	return false;
}

static void on_sof(void) {
	uint16_t now = (UFRMH << 8) | UFRML;
	BOOL changed = false;

	if (((now - button_change_frame) & 0x7FF) >= 500) {
		button_change_frame = now;
		intf0_buffer ^= 1;
		changed = true;
	}

	if (changed || (intf0_idle_rate != 0 && ((now - intf0_last_tx_frame) & 0x7FF) / 4 >= intf0_idle_rate)) {
		intf0_last_tx_frame = now;
		usb_bdpairs[1].in.BDSTATbits.cpu.UOWN = 1;
	}
}

static void on_in1(void) {
	if (usb_bdpairs[1].in.BDSTATbits.cpu.UOWN) {
		return;
	}
	if (intf0_halted) {
		return;
	}
	usb_bdpairs[1].in.BDCNT = 1;
	usb_bdpairs[1].in.BDADR = &intf0_buffer;
	if (usb_bdpairs[1].in.BDSTATbits.sie.OLDDTS) {
		usb_bdpairs[1].in.BDSTAT = BDSTAT_DTSEN;
	} else {
		usb_bdpairs[1].in.BDSTAT = BDSTAT_DTS | BDSTAT_DTSEN;
	}
}

static void on_enter_config1(void) {
	uint16_t now = (UFRMH << 8) | UFRML;
	intf0_buffer = 0;
	intf0_idle_rate = 0;
	intf0_last_tx_frame = 0;
	intf0_halted = 0;
	usb_ep_callbacks[1].in = &on_in1;
	usb_bdpairs[1].in.BDCNT = 1;
	usb_bdpairs[1].in.BDADR = &intf0_buffer;
	usb_bdpairs[1].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
	button_change_frame = now;
	usb_sof_callback = &on_sof;
	UEP1 = 0;
	UEP1bits.EPHSHK = 1;
	UEP1bits.EPINEN = 1;
	UEP1bits.EPCONDIS = 1;
	LAT_LED1 = 1;
}

static void on_exit_config1(void) {
	LAT_LED1 = 0;
	usb_sof_callback = 0;
	UEP1 = 0;
	usb_bdpairs[1].in.BDSTAT = 0;
	usb_ep_callbacks[1].in = 0;
}

static void on_endpoint_halt_config1(uint8_t ep) {
	switch (ep) {
		case 0x81:
			intf0_halted = true;
			usb_bdpairs[1].in.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
			break;
	}
}

static void on_endpoint_reinit_config1(uint8_t ep) {
	switch (ep) {
		case 0x81:
			intf0_halted = false;
			usb_bdpairs[1].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
			break;
	}
}

__code static const usb_confinfo_t config1 = {
	&CONFIGURATION_DESCRIPTOR,
	1,
	0x0002,
	0x0000,
	&on_enter_config1,
	&on_exit_config1,
	&on_endpoint_halt_config1,
	&on_endpoint_reinit_config1,
};

__code static const usb_devinfo_t devinfo = {
	&DEVICE_DESCRIPTOR,
	&custom_setup_handler,
	&STRING_DESCRIPTOR_ZERO,
	&STRING_METATABLE,
	{ &config1, },
};

/**
 * \brief The application entry point.
 */
void main(void) {
	/* Configure I/O pins. */
	PINS_INITIALIZE();

	/* Wait for the crystal oscillator to start. */
	while (!OSCCONbits.OSTS);

	/* Enable the PLL and wait for it to lock. This may take up to 2ms. */
	OSCTUNEbits.PLLEN = 1;
	delay1ktcy(2);

	/* Configure interrupt handling. */
	RCONbits.IPEN = 1;
	INTCONbits.GIEH = 1;
	INTCONbits.GIEL = 1;

	/* Initialize USB. */
	usb_init(&devinfo);

	for (;;) {
		if (usb_is_idle) {
			INTCONbits.GIEH = 0;
			if (usb_is_idle) {
				/* Disable the PLL. */
				OSCTUNEbits.PLLEN = 0;
				/* Go to sleep. */
				Sleep();
				/* Wait for the crystal oscillator to start back up. */
				while (!OSCCONbits.OSTS);
				/* Enable the PLL and wait for it to lock. This may take up to 2ms. */
				OSCTUNEbits.PLLEN = 1;
				delay1ktcy(2);
				
			}
			INTCONbits.GIEH = 1;
		}
	}
}

