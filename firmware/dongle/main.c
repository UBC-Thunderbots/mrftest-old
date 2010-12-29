#include "activity_leds.h"
#include "buffers.h"
#include "descriptors.h"
#include "dongle_proto_out.h"
#include "dongle_status.h"
#include "estop.h"
#include "local_error_queue.h"
#include "pins.h"
#include "serial.h"
#include "signal.h"
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
	TBOTS_CONTROL_REQUEST_GET_STATUS_BLOCK = 0x01,
	TBOTS_CONTROL_REQUEST_HALT_ALL = 0x02,
	TBOTS_CONTROL_REQUEST_ENABLE_RADIOS = 0x03,
} tbots_control_request_t;

/**
 * \brief Whether the USB layer has entered the main configuration and the main loop should start configuring XBees and processing packets.
 */
static volatile BOOL should_start_up = false;

/**
 * \brief Whether the USB layer has exited the main configuration and the main loop should stop processing packets and shut down the XBees.
 */
static volatile BOOL should_shut_down = false;

/**
 * \brief The channels the host asked the XBees to be configured on.
 */
static volatile uint8_t requested_channels[2] = { 0, 0 };

/**
 * \brief The firmware versions of the XBees.
 */
static uint16_t xbee_versions[2];

/**
 * \brief The most recent state blocks sent to the drive pipe.
 */
static uint8_t drive_states[15][9];

/**
 * \brief Packet structures suitable for XBee packet reception.
 */
static __data xbee_rxpacket_t xbee_packets[NUM_XBEE_BUFFERS];

/**
 * \brief Blocks of memory in which inbound USB transfers are assembled.
 */
static __data uint8_t dongle_proto_in_buffers[2][64];

DEF_INTHIGH(high_handler)
	__asm extern _xbee_rxpacket_rc1if __endasm;
	__asm extern _xbee_rxpacket_rc2if __endasm;
	DEF_HANDLER2(SIG_RC1, SIG_RC1IE, xbee_rxpacket_rc1if)
	DEF_HANDLER2(SIG_RC2, SIG_RC2IE, xbee_rxpacket_rc2if)
END_DEF

DEF_INTLOW(low_handler)
	__asm extern _usb_process __endasm;
	__asm extern _estop_adif __endasm;
	__asm extern _xbee_txpacket_tx1if __endasm;
	__asm extern _xbee_txpacket_tx2if __endasm;
	__asm extern _xbee_txpacket_ccp1if __endasm;
	__asm extern _activity_leds_tmr0if __endasm;
	DEF_HANDLER2(SIG_USB, SIG_USBIE, usb_process)
	DEF_HANDLER2(SIG_AD, SIG_ADIE, estop_adif)
	DEF_HANDLER2(SIG_TX1, SIG_TX1IE, xbee_txpacket_tx1if)
	DEF_HANDLER2(SIG_TX2, SIG_TX2IE, xbee_txpacket_tx2if)
	DEF_HANDLER2(SIG_CCP1, SIG_CCP1IE, xbee_txpacket_ccp1if)
	DEF_HANDLER(SIG_TMR0, activity_leds_tmr0if)
END_DEF

#define IS_VALID_CHANNEL(ch) ((ch) >= 0x08 && (ch) <= 0x1A)

static BOOL custom_setup_handler(void) {
	static union {
		dongle_status_t ep0_dongle_status_buffer;
	} buffer;
	uint8_t i;

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

				case TBOTS_CONTROL_REQUEST_GET_STATUS_BLOCK:
					memcpyram2ram(&buffer.ep0_dongle_status_buffer, &dongle_status, sizeof(buffer.ep0_dongle_status_buffer));
					usb_ep0_data[0].ptr = &buffer.ep0_dongle_status_buffer;
					usb_ep0_data[0].length = sizeof(buffer.ep0_dongle_status_buffer);
					usb_ep0_data_length = 1;
					return true;

				case TBOTS_CONTROL_REQUEST_HALT_ALL:
					for (i = 0; i < 15; ++i) {
						drive_states[i][0] = 0;
					}
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
					}
					return false;

			}
		}
	}
	return false;
}

static void on_in3(void) {
}

static void on_in4(void) {
}

static void on_in5(void) {
}

static void on_enter_config1(void) {
	dongle_status_start();
	local_error_queue_init();
	dongle_proto_out_init();
	usb_ep_callbacks[3].in = &on_in3;
	usb_ep_callbacks[4].in = &on_in4;
	usb_ep_callbacks[5].in = &on_in5;
	should_start_up = true;
}

static void on_exit_config1(void) {
	dongle_proto_out_deinit();
	local_error_queue_deinit();
	dongle_status_stop();
	should_start_up = false;
	should_shut_down = true;
	requested_channels[0] = requested_channels[1] = 0;
}

static void on_endpoint_halt_config1(uint8_t ep) {
	switch (ep) {
		case 0x04:
		case 0x05:
			dongle_proto_out_halt(ep);
			break;

		case 0x81:
			dongle_status_halt();
			break;

		case 0x82:
			local_error_queue_halt();
			break;

		case 0x83:
		case 0x84:
		case 0x85:
#warning implement
			break;
	}
}

static BOOL on_endpoint_unhalt_config1(uint8_t ep) {
	switch (ep) {
		case 0x04:
		case 0x05:
			dongle_proto_out_unhalt(ep);
			return true;

		case 0x81:
			dongle_status_unhalt();
			return true;

		case 0x82:
			local_error_queue_unhalt();
			return true;

		case 0x83:
		case 0x84:
		case 0x85:
#warning implement
			return true;
	}
	return false;
}

__code static const usb_confinfo_t config1 = {
	&CONFIGURATION_DESCRIPTOR,
	2,
	0b0000000000111110,
	0b0000000000110000,
	&on_enter_config1,
	&on_exit_config1,
	&on_endpoint_halt_config1,
	&on_endpoint_unhalt_config1,
};

__code static const usb_devinfo_t devinfo = {
	&DEVICE_DESCRIPTOR,
	&custom_setup_handler,
	&STRING_DESCRIPTOR_ZERO,
	&STRING_METATABLE,
	{ &config1, },
};

/**
 * \brief Checks if the USB subsystem has become idle and, if so, put the dongle to sleep until the USB host comes back.
 */
static void check_idle(void) {
	uint8_t saved_stuff;

	/* Double-checked locking paradigm:
	 * There might be a race where we were idle but become active right after the check.
	 * The interrupt would be taken and clear usb_is_idle, but we'd put the MCU to sleep because we were already in the if.
	 * This would be bad, because we'd put the MCU to sleep while the SIE was not suspended!
	 * Instead, if we think we're idle, disable interrupts and check again.
	 * Only if that second check also passes do we actually go to sleep.
	 * Note that we never re-enable GIEH during the body of the if.
	 * Therefore, bus activity will cause ACVTIF to become set but no interrupt will be taken immediately.
	 * The SLEEP instruction does not go to sleep if any interrupt is pending, or wakes up if any interrupt becomes pending.
	 * That is the "atomic" instruction in this system: it guarantees to sleep only if, and as long as, no interrupt is pending.
	 * Once ACTVIF is pending, we wake up, bring up the hardware, and re-enable GIEH.
	 * Only then is the interrupt taken. */
	if (usb_is_idle) {
		INTCONbits.GIEH = 0;
		if (usb_is_idle) {
			/* Flush and suspend communication. */
			xbee_txpacket_suspend();
			xbee_rxpacket_suspend();

			/* Save the states of and turn off all LEDs and the XBees. */
			saved_stuff = 0;
			if (LAT_LED1) {
				saved_stuff |= 1;
			}
			if (LAT_LED2) {
				saved_stuff |= 2;
			}
			if (LAT_LED3) {
				saved_stuff |= 4;
			}
			if (!LAT_XBEE0_SLEEP) {
				saved_stuff |= 8;
			}
			if (!LAT_XBEE1_SLEEP) {
				saved_stuff |= 16;
			}
			LAT_LED1 = 0;
			LAT_LED2 = 0;
			LAT_LED3 = 0;
			LAT_XBEE0_SLEEP = 1;
			LAT_XBEE1_SLEEP = 1;

			/* Disable the PLL. */
			OSCTUNEbits.PLLEN = 0;

			/* Go to sleep. */
			Sleep();

			/* Wait for the crystal oscillator to start back up. */
			while (!OSCCONbits.OSTS);

			/* Enable the PLL and wait for it to lock. This may take up to 2ms. */
			OSCTUNEbits.PLLEN = 1;
			delay1ktcy(2);

			/* Turn back on all the hardware we turned off.
			 * As described above, this will not race with things like SET_CONFIGURATION callbacks.
			 * Reason: WE HAVE NOT YET TAKEN THE INTERRUPT THAT WOKE US UP.
			 * The entire body of this if block is a critical section! */
			if (saved_stuff & 1) {
				LAT_LED1 = 1;
			}
			if (saved_stuff & 2) {
				LAT_LED2 = 1;
			}
			if (saved_stuff & 4) {
				LAT_LED3 = 1;
			}
			if (saved_stuff & 8) {
				LAT_XBEE0_SLEEP = 0;
			}
			if (saved_stuff & 16) {
				LAT_XBEE1_SLEEP = 0;
			}

			/* Restart communication. */
			xbee_txpacket_resume();
			xbee_rxpacket_resume();
		}
		INTCONbits.GIEH = 1;
	}
}

/**
 * \brief Checks for low-level XBee errors and, if any are present, prepares to report them to the host.
 */
static void check_xbee_errors(void) {
	uint8_t mask, i;

	for (i = 0; i != 2; ++i) {
		mask = xbee_rxpacket_errors(i);
		if (mask & (1 << XBEE_RXPACKET_ERROR_FERR)) {
			local_error_queue_add(1 + i);
		}
		if (mask & (1 << XBEE_RXPACKET_ERROR_OERR_HW)) {
			local_error_queue_add(3 + i);
		}
		if (mask & (1 << XBEE_RXPACKET_ERROR_OERR_SW)) {
			local_error_queue_add(5 + i);
		}
		if (mask & (1 << XBEE_RXPACKET_ERROR_CHECKSUM_ERROR)) {
			local_error_queue_add(7 + i);
		}
		if (mask & (1 << XBEE_RXPACKET_ERROR_LENGTH_MSB_NONZERO)) {
			local_error_queue_add(9 + i);
		}
		if (mask & (1 << XBEE_RXPACKET_ERROR_LENGTH_LSB_ILLEGAL)) {
			local_error_queue_add(11 + i);
		}
	}
}

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
static void at_send(uint8_t xbee, uint8_t frame, __code const char *command, const void *value, uint8_t val_length) {
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
		check_idle();
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
			if ((rxpkt = xbee_rxpacket_dequeue())) {
				/* See which XBee this packet came from. */
				if (rxpkt->xbee == xbee) {
					/* We received a packet. See what it is. */
					if (rxpkt->len == 5 + resp_length && rxpkt->ptr[0] == 0x88 && rxpkt->ptr[1] == frame) {
						/* It's an AT command response whose frame ID matches ours. */
						success = false;
						if (rxpkt->ptr[2] != command[0] || rxpkt->ptr[3] != command[1]) {
							/* The command is wrong. */
							local_error_queue_add(13 + xbee); /* XBee {0,1} AT command failure, response contains incorrect command */
						} else if (rxpkt->ptr[4] != 0) {
							/* The response is a failure. */
							switch (rxpkt->ptr[4]) {
								case 2:
									local_error_queue_add(17 + xbee); /* XBee {0,1} AT command failure, invalid command */
									break;

								case 3:
									local_error_queue_add(19 + xbee); /* XBee {0,1} AT command failure, invalid parameter */
									break;

								default:
									local_error_queue_add(15 + xbee); /* XBee {0,1} AT command failure, unknown reason */
									break;
							}
						} else {
							/* The command succeeded. */
							success = true;
							memcpyram2ram(resp, rxpkt->ptr + 5, resp_length);
						}
						xbee_rxpacket_queue(rxpkt);
						return success;
					} else {
						/* It's not a response to our request. Keep waiting. */
						xbee_rxpacket_queue(rxpkt);
					}
				} else {
					/* This packet came from the wrong XBee. Ignore it. */
					xbee_rxpacket_queue(rxpkt);
				}
			}
			check_xbee_errors();
			check_idle();
		}
	}

	/* Out of retries. Give up. */
	local_error_queue_add(21 + xbee); /* XBee {0,1} timeout waiting for local modem response */
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
	BOOL success = false;
	uint8_t buffer[2];

	/* Mark status. */
	dongle_status.xbees = XBEES_STATE_INIT1_0 + xbee;
	dongle_status_dirty();

	/* Reset the modem. */
	if (!at_command(xbee, 0x81, "FR", 0, 0, 0, 0)) {
		local_error_queue_add(23 + xbee);
		goto out;
	}

	/* Enable RTS flow control on pin DIO6. */
	buffer[0] = 1;
	if (!at_command(xbee, 0x82, "D6", buffer, 1, 0, 0)) {
		local_error_queue_add(25 + xbee);
		goto out;
	}

	/* Retrieve the firmware version. */
	if (!at_command(xbee, 0x83, "VR", 0, 0, buffer, 2)) {
		local_error_queue_add(27 + xbee);
		goto out;
	}
	xbee_versions[xbee] = (buffer[0] << 8) | buffer[1];

	/* Set up the PAN ID. */
	buffer[0] = 0x49;
	buffer[1] = 0x6C + xbee;
	if (!at_command(xbee, 0x84, "ID", buffer, 2, 0, 0)) {
		local_error_queue_add(29 + xbee);
		goto out;
	}

	/* Set up the text node ID. */
	if (!at_command(xbee, 0x85, "NI", (xbee == 0) ? "TBOTS00" : "TBOTS01", 7, 0, 0)) {
		local_error_queue_add(31 + xbee);
		goto out;
	}

	success = true;

out:
	/* Mark final status. */
	if (success) {
		if (xbee == 1) {
			dongle_status.xbees = XBEES_STATE_INIT1_DONE;
		}
	} else {
		dongle_status.xbees = XBEES_STATE_FAIL_0 + xbee;
	}
	dongle_status_dirty();
	return success;
}

/**
 * \brief Stage-2 configures an XBee.
 *
 * \param[in] xbee the index of the XBee to configure.
 *
 * \return \c true on success, or \c false on failure.
 */
static BOOL configure_xbee_stage2(uint8_t xbee) {
	BOOL success = false;
	uint8_t buffer[2];

	/* Mark status. */
	dongle_status.xbees = XBEES_STATE_INIT2_0 + xbee;
	dongle_status_dirty();

	/* Set the radio channel. */
	if (!at_command(xbee, 0x90, "CH", &requested_channels[xbee], 1, 0, 0)) {
		local_error_queue_add(33 + xbee);
		goto out;
	}

	/* Set the 16-bit address. */
	buffer[0] = 0x7B;
	buffer[1] = (xbee == 0) ? 0x20 : 0x30;
	if (!at_command(xbee, 0x91, "MY", buffer, 2, 0, 0)) {
		local_error_queue_add(35 + xbee);
		goto out;
	}

	success = true;

out:
	/* Mark final status. */
	if (success) {
		if (xbee == 1) {
			dongle_status.xbees = XBEES_STATE_RUNNING;
		}
	} else {
		dongle_status.xbees = XBEES_STATE_FAIL_0 + xbee;
	}
	dongle_status_dirty();
	return success;
}

/**
 * \brief The application entry point.
 */
void main(void) {
	uint8_t i;

	/* Configure I/O pins. */
	PINS_INITIALIZE();
	WDTCONbits.ADSHR = 1;
	ANCON0 = 0x01;
	ANCON1 = 0x00;
	WDTCONbits.ADSHR = 0;

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
			check_idle();
		} while (!should_start_up);

		/* Bring up the hardware. */
		estop_init();
		serial_init();
		xbee_txpacket_init();
		xbee_rxpacket_init();
		LAT_XBEE0_SLEEP = 0;
		LAT_XBEE1_SLEEP = 0;
		LAT_LED1 = 1;

		/* Give the receive buffers to the XBees. */
		for (i = 0; i < NUM_XBEE_BUFFERS; ++i) {
			xbee_packets[i].ptr = xbee_buffers[i];
			xbee_rxpacket_queue(&xbee_packets[i]);
		}

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
			while (!should_shut_down) {
				check_idle();
			}
		} else {
			/* Configuration succeeded.
			 * Wait for the host to give us radio channels. */
			while (!requested_channels[0] && !should_shut_down) {
				check_idle();
			}
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
					while (!should_shut_down) {
						check_idle();
					}
				} else {
#warning test code
					activity_leds_init();
					while (!should_shut_down) {
						check_idle();
					}
					activity_leds_deinit();
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
		estop_deinit();
		LAT_LED1 = 0;
		LAT_LED2 = 0;
		LAT_LED3 = 0;
	}
}

