#include "critsec.h"
#include "debug.h"
#include "descriptors.h"
#include "dongle_status.h"
#include "endpoints.h"
#include "error_reporting.h"
#include "estop.h"
#include "global.h"
#include "message_in.h"
#include "message_out.h"
#include "pins.h"
#include "run.h"
#include "serial.h"
#include "signal.h"
#include "state_transport_in.h"
#include "state_transport_out.h"
#include "usb.h"
#include "xbee_rxpacket.h"
#include "xbee_txpacket.h"
#include <delay.h>
#include <pic18fregs.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * \file
 *
 * \brief Contains the application entry point.
 */

/**
 * \brief The control transfer requests.
 */
typedef enum {
	TBOTS_CONTROL_REQUEST_GET_XBEE_FW_VERSION = 0x00,
	TBOTS_CONTROL_REQUEST_GET_XBEE_CHANNELS = 0x01,
	TBOTS_CONTROL_REQUEST_ENABLE_RADIOS = 0x02,
} tbots_control_request_t;

DEF_INTHIGH(high_handler)
	__asm extern _xbee_rxpacket_rc1if __endasm;
	__asm extern _xbee_rxpacket_rc2if __endasm;
	DEF_HANDLER(SIG_RC1, xbee_rxpacket_rc1if)
	DEF_HANDLER(SIG_RC2, xbee_rxpacket_rc2if)
END_DEF

DEF_INTLOW(low_handler)
	__asm extern _state_transport_out_tmr1if __endasm;
	__asm extern _usb_process __endasm;
	__asm extern _estop_adif __endasm;
	__asm extern _xbee_txpacket_tx1if __endasm;
	__asm extern _xbee_txpacket_tx2if __endasm;
	__asm extern _xbee_txpacket_tmr4if __endasm;
	DEF_HANDLER(SIG_TMR1, state_transport_out_tmr1if)
	DEF_HANDLER2(SIG_USB, SIG_USBIE, usb_process)
	DEF_HANDLER(SIG_AD, estop_adif)
	DEF_HANDLER2(SIG_TX1, SIG_TX1IE, xbee_txpacket_tx1if)
	DEF_HANDLER2(SIG_TX2, SIG_TX2IE, xbee_txpacket_tx2if)
	DEF_HANDLER(SIG_TMR4, xbee_txpacket_tmr4if)
END_DEF

#define IS_VALID_CHANNEL(ch) ((ch) >= 0x08 && (ch) <= 0x1A)

static BOOL custom_setup_handler(void) {
	static union {
		uint8_t debug_interface_alt_setting;
	} buffer;

	if (usb_ep0_setup_buffer.request_type.bits.type == USB_SETUP_PACKET_REQUEST_VENDOR) {
		if (usb_ep0_setup_buffer.request_type.bits.recipient == USB_SETUP_PACKET_RECIPIENT_DEVICE) {
			switch (usb_ep0_setup_buffer.request) {
				case TBOTS_CONTROL_REQUEST_GET_XBEE_FW_VERSION:
					if (dongle_status.xbees >= XBEES_STATE_INIT1_DONE && xbee_versions[0] && xbee_versions[1]) {
						usb_ep0_data[0].ptr = xbee_versions;
						usb_ep0_data[0].length = sizeof(xbee_versions);
						usb_ep0_data_length = 1;
						return true;
					} else {
						return false;
					}

				case TBOTS_CONTROL_REQUEST_GET_XBEE_CHANNELS:
					usb_ep0_data[0].ptr = requested_channels;
					usb_ep0_data[0].length = sizeof(requested_channels);
					usb_ep0_data_length = 1;
					return true;

				case TBOTS_CONTROL_REQUEST_ENABLE_RADIOS:
					if (dongle_status.xbees == XBEES_STATE_INIT1_DONE && !requested_channels[0]) {
						requested_channels[0] = usb_ep0_setup_buffer.value & 0xFF;
						requested_channels[1] = usb_ep0_setup_buffer.value >> 8;
						if (IS_VALID_CHANNEL(requested_channels[0]) && IS_VALID_CHANNEL(requested_channels[1])) {
							return true;
						} else {
							requested_channels[0] = requested_channels[1] = 0;
						}
					} else if (requested_channels[0] == (usb_ep0_setup_buffer.value & 0xFF) && requested_channels[1] == (usb_ep0_setup_buffer.value >> 8)) {
						return true;
					}
					return false;

			}
		}
	}
	return false;
}

static void on_enter_config1(void) {
	dongle_status_start();
	estop_init();
	error_reporting_init();
	state_transport_out_init();
	state_transport_in_init();
	message_out_init();
	message_in_init();
	debug_enable();
	should_start_up = true;
}

static void on_exit_config1(void) {
	debug_disable();
	message_in_deinit();
	message_out_deinit();
	state_transport_in_deinit();
	state_transport_out_deinit();
	error_reporting_deinit();
	estop_deinit();
	dongle_status_stop();
	should_start_up = false;
	should_shut_down = true;
	requested_channels[0] = requested_channels[1] = 0;
}

__code static const usb_confinfo_t config1 = {
	&CONFIGURATION_DESCRIPTOR,
	2,
	(1 << EP_DONGLE_STATUS) | (1 << EP_LOCAL_ERROR_QUEUE) | (1 << EP_STATISTICS) | (1 << EP_STATE_TRANSPORT) | (1 << EP_INTERRUPT) | (1 << EP_DEBUG),
	(1 << EP_STATE_TRANSPORT) | (1 << EP_INTERRUPT),
	&on_enter_config1,
	&on_exit_config1,
};

__code static const usb_devinfo_t devinfo = {
	&DEVICE_DESCRIPTOR,
	&custom_setup_handler,
	&STRING_DESCRIPTOR_ZERO,
	&STRING_METATABLE,
	{ &config1, },
};

/**
 * \brief Constructs an AT Command packet and sends it to an XBee.
 *
 * \param[in] xbee the index of the XBee to send the command to.
 *
 * \param[in] frame the frame number to use.
 *
 * \param[in] command the AT command to send.
 *
 * \param[in] value the value to set the parameter to.
 *
 * \param[in] val_length the number of bytes to use to represent \p value.
 */
static void at_send(uint8_t xbee, uint8_t frame, __code const char *command, __data const void *value, uint8_t val_length) {
	uint8_t header[4];
	xbee_txpacket_iovec_t txiovs[2];
	xbee_txpacket_t txpkt;

	header[0] = 0x08;
	header[1] = frame;
	header[2] = command[0];
	header[3] = command[1];

	txiovs[0].ptr = header;
	txiovs[0].len = sizeof(header);
	txiovs[1].ptr = value;
	txiovs[1].len = val_length;

	txpkt.num_iovs = 2;
	txpkt.iovs = txiovs;

	xbee_txpacket_queue(&txpkt, xbee);

	while (xbee_txpacket_dequeue() != &txpkt) {
		if (should_shut_down) {
			return;
		}
	}
}

/**
 * \brief Issues an AT command to an XBee to get or set a parameter.
 *
 * \param[in] xbee the index of the XBee to send the command to.
 *
 * \param[in] frame the frame number to use.
 *
 * \param[in] command the AT command to send.
 *
 * \param[in] value the value to set the parameter to.
 *
 * \param[in] val_length the number of bytes to use to represent \p value.
 *
 * \param[out] resp the location to store the returned value in.
 *
 * \param[in] resp_length the size of response to expect.
 *
 * \return \c true on success, or \c false on failure.
 */
static BOOL at_command(uint8_t xbee, uint8_t frame, __code const char *command, const void *value, uint8_t val_length, __data void *resp, uint8_t resp_length) {
	uint8_t retries, start_time;
	__data xbee_rxpacket_t *rxpkt;
	BOOL success;

	for (retries = 0; retries != 6 && !should_shut_down; ++retries) {
		/* Send the command. */
		at_send(xbee, frame, command, value, val_length);

		/* Wait for a response for up to half a second. */
		start_time = UFRMH;
		while (((UFRMH - start_time) & 7) < 2 && !should_shut_down) {
			if ((rxpkt = xbee_rxpacket_get())) {
				/* See which XBee this packet came from. */
				if (rxpkt->xbee == xbee) {
					/* We received a packet. See what it is. */
					if (rxpkt->len == 5 + resp_length && rxpkt->buf[0] == 0x88 && rxpkt->buf[1] == frame) {
						/* It's an AT command response whose frame ID matches ours. */
						success = false;
						if ((char) rxpkt->buf[2] != command[0] || (char) rxpkt->buf[3] != command[1]) {
							/* The command is wrong. */
							error_reporting_add(FAULT_XBEE0_AT_RESPONSE_WRONG_COMMAND + xbee);
						} else if (rxpkt->buf[4] != 0) {
							/* The response is a failure. */
							switch (rxpkt->buf[4]) {
								case 2:
									error_reporting_add(FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_COMMAND + xbee);
									break;

								case 3:
									error_reporting_add(FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_PARAMETER + xbee);
									break;

								default:
									error_reporting_add(FAULT_XBEE0_AT_RESPONSE_FAILED_UNKNOWN_REASON + xbee);
									break;
							}
						} else {
							/* The command succeeded. */
							success = true;
							memcpyram2ram(resp, rxpkt->buf + 5, resp_length);
						}
						xbee_rxpacket_free(rxpkt);
						return success;
					} else {
						/* It's not a response to our request. Keep waiting. */
						xbee_rxpacket_free(rxpkt);
					}
				} else {
					/* This packet came from the wrong XBee. Ignore it. */
					xbee_rxpacket_free(rxpkt);
				}
			}
		}
	}

	/* Out of retries. Give up. */
	if (!should_shut_down) {
		error_reporting_add(FAULT_XBEE0_TIMEOUT + xbee); /* XBee {0,1} timeout waiting for local modem response */
	}
	return false;
}

/**
 * \brief Stage-1 configures an XBee.
 *
 * \param[in] xbee the index of the XBee to configure.
 *
 * \return \c true on success, or \c false on failure.
 */
static BOOL configure_xbee_stage1(uint8_t xbee) {
	uint8_t buffer[2];
	uint8_t err = 0;
	CRITSEC_DECLARE(cs);

	/* Mark status. */
	CRITSEC_ENTER_LOW(cs);
	if (should_shut_down) {
		CRITSEC_LEAVE(cs);
		goto out;
	}
	dongle_status.xbees = XBEES_STATE_INIT1_0 + xbee;
	CRITSEC_LEAVE(cs);
	dongle_status_dirty();

	/* Reset the modem. */
	if (!at_command(xbee, 0x81, "FR", 0, 0, 0, 0)) {
		err = FAULT_XBEE0_RESET_FAILED + xbee;
		goto out;
	}

	/* Enable RTS flow control on pin DIO6. */
	buffer[0] = 1;
	if (!at_command(xbee, 0x82, "D6", buffer, 1, 0, 0)) {
		err = FAULT_XBEE0_ENABLE_RTS_FAILED + xbee;
		goto out;
	}

	/* Retrieve the firmware version. */
	if (!at_command(xbee, 0x83, "VR", 0, 0, buffer, 2)) {
		err = FAULT_XBEE0_GET_FW_VERSION_FAILED + xbee;
		goto out;
	}
	xbee_versions[xbee] = (buffer[0] << 8) | buffer[1];

	/* Set up the text node ID. */
	if (!at_command(xbee, 0x85, "NI", (xbee == 0) ? "TBOTSD0" : "TBOTSD1", 7, 0, 0)) {
		err = FAULT_XBEE0_SET_NODE_ID_FAILED + xbee;
		goto out;
	}

out:
	/* Mark final status. */
	CRITSEC_ENTER_LOW(cs);
	if (!should_shut_down) {
		if (!err) {
			if (xbee == 1) {
				dongle_status.xbees = XBEES_STATE_INIT1_DONE;
			}
		} else {
			error_reporting_add(err);
			dongle_status.xbees = XBEES_STATE_FAIL_0 + xbee;
		}
	}
	CRITSEC_LEAVE(cs);
	dongle_status_dirty();
	return !err;
}

/**
 * \brief Stage-2 configures an XBee.
 *
 * \param[in] xbee the index of the XBee to configure.
 *
 * \return \c true on success, or \c false on failure.
 */
static BOOL configure_xbee_stage2(uint8_t xbee) {
	uint8_t buffer[2];
	uint8_t err = 0;
	CRITSEC_DECLARE(cs);

	/* Mark status. */
	CRITSEC_ENTER_LOW(cs);
	if (should_shut_down) {
		CRITSEC_LEAVE(cs);
		goto out;
	}
	dongle_status.xbees = XBEES_STATE_INIT2_0 + xbee;
	CRITSEC_LEAVE(cs);
	dongle_status_dirty();

	/* Set the radio channel. */
	if (!at_command(xbee, 0x90, "CH", &requested_channels[xbee], 1, 0, 0)) {
		err = FAULT_XBEE0_SET_CHANNEL_FAILED + xbee;
		goto out;
	}

	/* Set up the PAN ID. */
	buffer[0] = 0x49;
	buffer[1] = 0x6C + xbee;
	if (!at_command(xbee, 0x84, "ID", buffer, 2, 0, 0)) {
		err = FAULT_XBEE0_SET_PAN_ID_FAILED + xbee;
		goto out;
	}

	/* Set the 16-bit address. */
	buffer[0] = 0x7B;
	buffer[1] = 0x40 + xbee;
	if (!at_command(xbee, 0x91, "MY", buffer, 2, 0, 0)) {
		err = FAULT_XBEE0_SET_ADDRESS_FAILED + xbee;
		goto out;
	}

out:
	/* Mark final status. */
	CRITSEC_ENTER_LOW(cs);
	if (!should_shut_down) {
		if (!err) {
			if (xbee == 1) {
				dongle_status.xbees = XBEES_STATE_RUNNING;
			}
		} else {
			error_reporting_add(err);
			dongle_status.xbees = XBEES_STATE_FAIL_0 + xbee;
		}
	}
	CRITSEC_LEAVE(cs);
	dongle_status_dirty();
	return !err;
}

/**
 * \brief The application entry point.
 */
void main(void) {
	/* Configure I/O pins. */
	PINS_INITIALIZE();
	WDTCONbits.ADSHR = 1;
	ANCON0 = 0xFE;
	ANCON1 = 0xFF;
	WDTCONbits.ADSHR = 0;

	/* Initialize the debug output subsystem. */
	debug_init();

	/* Disable timer 0 until needed. */
	T0CONbits.TMR0ON = 0;

	/* We want to attach timer 1 to ECCP 1 and timer 3 to ECCP2 so we can have separate periods. */
	T3CONbits.T3CCP2 = 0;
	T3CONbits.T3CCP1 = 1;

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

	/* The rest of the function is a giant state loop.
	 * We transition through a state machine linearly as follows:
	 * 1. Waiting for the host to perform USB configuration.
	 * 2. Waiting for the XBees to be configured and ready to use.
	 *    2a. Stopping configuration and notifying the host of an unrecoverable error.
	 * 3. Performing main communication. */
	for (;;) {
		/* We're waiting for the host to perform USB configuration. */
		do {
			should_shut_down = false;
		} while (!should_start_up);

		/* Bring up the hardware. */
		LAT_XBEE0_SLEEP = 0;
		LAT_XBEE1_SLEEP = 0;
		serial_init();
		xbee_txpacket_init();
		xbee_rxpacket_init();

		/* Clear states to safe values. */
		memset(state_transport_out_drive, 0, sizeof(state_transport_out_drive));
		memset(state_transport_in_feedback, 0, sizeof(state_transport_in_feedback));

		/* Stage-1 configure the XBees. */
		if (!configure_xbee_stage1(0) || !configure_xbee_stage1(1)) {
			/* Configuration failed.
			 * Put the XBees back to sleep. */
			LAT_XBEE0_SLEEP = 1;
			LAT_XBEE1_SLEEP = 1;
			/* Represent this by turning the green LED off and the red LED for the failed XBee on. */
			LAT_LED1 = 0;
			if (dongle_status.xbees == XBEES_STATE_FAIL_0) {
				LAT_LED2 = 1;
			} else {
				LAT_LED3 = 1;
			}
			/* Stay here until the host signals us to shut down. */
			while (!should_shut_down);
		} else {
			/* Configuration succeeded.
			 * Wait for the host to give us radio channels. */
			while (!requested_channels[0] && !should_shut_down);
			if (requested_channels[0]) {
				/* Stage-2 configure the XBees. */
				if (!configure_xbee_stage2(0) || !configure_xbee_stage2(1)) {
					/* Configuration failed.
					 * Put the XBees back to sleep. */
					LAT_XBEE0_SLEEP = 1;
					LAT_XBEE1_SLEEP = 1;
					/* Represent this by turning the green LED off and the red LED for the failed XBee on. */
					LAT_LED1 = 0;
					if (dongle_status.xbees == XBEES_STATE_FAIL_0) {
						LAT_LED2 = 1;
					} else {
						LAT_LED3 = 1;
					}
					/* Stay here until the host signals us to shut down. */
					while (!should_shut_down);
				} else {
					LAT_LED1 = 1;
					run();
				}
			}
		}

		/* Bring down the hardware. */
		dongle_status.xbees = XBEES_STATE_PREINIT;
		dongle_status.robots = 0;
		LAT_XBEE0_SLEEP = 0;
		LAT_XBEE1_SLEEP = 0;
		xbee_rxpacket_deinit();
		xbee_txpacket_deinit();
		serial_deinit();
		LAT_LED1 = 0;
		LAT_LED2 = 0;
		LAT_LED3 = 0;
	}
}

