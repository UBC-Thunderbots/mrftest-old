#include "state_transport_out.h"
#include "dongle_status.h"
#include "endpoints.h"
#include "error_reporting.h"
#include "pipes.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <string.h>

/**
 * \brief The number of timer 1 rollovers (~44ms each) before an application crash is assumed and the robots are scrammed.
 *
 * Approximately 1 second.
 */
#define SCRAM_TIMEOUT_LIMIT 23

__data drive_block_t state_transport_out_drive[15];

/**
 * \brief A buffer into which packets are received.
 */
static uint8_t buffer[64];

/**
 * \brief The number of timer 1 rollovers left before an application crash is assumed and the robots are scrammed.
 */
static volatile uint8_t scram_timeout = 0;

static void on_transaction(void) {
	uint8_t recipient;
	__data const uint8_t *ptr = buffer;
	uint8_t left = USB_BD_OUT_RECEIVED(EP_STATE_TRANSPORT);

	/* Clear the timeout. */
	scram_timeout = SCRAM_TIMEOUT_LIMIT;

	/* Decode the packet. */
	while (left) {
		if (ptr[0] <= left) {
			switch (ptr[1] & 0x0F) {
				case PIPE_DRIVE:
					/* Drive pipe. */
					if (ptr[0] == sizeof(drive_block_t) + 2) {
						recipient = ptr[1] >> 4;
						if (recipient) {
							/* Directed. */
							memcpyram2ram(&state_transport_out_drive[recipient - 1], ptr + 2, sizeof(drive_block_t));
						} else {
							/* Broadcast. */
							for (recipient = 0; recipient != 15; ++recipient) {
								memcpyram2ram(&state_transport_out_drive[recipient], ptr + 2, sizeof(drive_block_t));
							}
						}
					} else {
						error_reporting_add(FAULT_OUT_MICROPACKET_BAD_LENGTH);
					}
					break;

				default:
					/* No such pipe. */
					error_reporting_add(FAULT_OUT_MICROPACKET_NOPIPE);
					break;
			}
			left -= ptr[0];
			ptr += ptr[0];
		} else {
			error_reporting_add(FAULT_OUT_MICROPACKET_OVERFLOW);
			break;
		}
	}

	if (dongle_status.xbees == XBEES_STATE_RUNNING) {
		/* Prepare to receive the next packet. */
		USB_BD_OUT_SUBMIT(EP_STATE_TRANSPORT, buffer, sizeof(buffer));
	} else if (USB_BD_OUT_CAN_FUNCTIONAL_STALL(EP_STATE_TRANSPORT)) {
		/* Emergency stop switch is not in RUN state.
		 * Set the endpoint halt feature. */
		usb_halted_out_endpoints |= 1 << EP_STATE_TRANSPORT;
		USB_BD_OUT_FUNCTIONAL_STALL(EP_STATE_TRANSPORT);
	}
}

static void on_commanded_stall(void) {
	uint8_t i;

	USB_BD_OUT_COMMANDED_STALL(EP_STATE_TRANSPORT);

	for (i = 0; i != 15; ++i) {
		state_transport_out_drive[i].flags.enable_robot = false;
	}
}

static BOOL on_clear_halt(void) {
	/* Halt status can only be cleared once XBee stage 2 configuration completes. */
	if (dongle_status.xbees == XBEES_STATE_RUNNING) {
		USB_BD_OUT_UNSTALL(EP_STATE_TRANSPORT);
		USB_BD_OUT_SUBMIT(EP_STATE_TRANSPORT, buffer, sizeof(buffer));
		return true;
	} else {
		return false;
	}
}

void state_transport_out_init(void) {
	/* Start up timer 1 to do the scram timeout.
	 *        /-------- Read/write in two 8-bit operations
	 *        |/------- Read-only
	 *        ||//----- 1:8 prescale
	 *        ||||/---- Oscillator block disabled
	 *        |||||/--- Ignored
	 *        ||||||/-- Internal clock source
	 *        |||||||/- Timer enabled */
	T1CON = 0b00110011;
	IPR1bits.TMR1IP = 1;
	PIE1bits.TMR1IE = 1;

	/* The endpoint is halted until XBee stage 2 configuration completes. */
	usb_halted_out_endpoints |= 1 << EP_STATE_TRANSPORT;
	usb_ep_callbacks[EP_STATE_TRANSPORT].out.transaction = &on_transaction;
	usb_ep_callbacks[EP_STATE_TRANSPORT].out.commanded_stall = &on_commanded_stall;
	usb_ep_callbacks[EP_STATE_TRANSPORT].out.clear_halt = &on_clear_halt;
	USB_BD_OUT_INIT(EP_STATE_TRANSPORT);
	USB_BD_OUT_FUNCTIONAL_STALL(EP_STATE_TRANSPORT);
	UEPBITS(EP_STATE_TRANSPORT).EPHSHK = 1;
	UEPBITS(EP_STATE_TRANSPORT).EPOUTEN = 1;
}

void state_transport_out_deinit(void) {
	UEPBITS(EP_STATE_TRANSPORT).EPOUTEN = 0;
}

void state_transport_out_tmr1if(void) {
	if (scram_timeout) {
		--scram_timeout;
	} else {
		uint8_t i;
		for (i = 0; i != 15; ++i) {
			state_transport_out_drive[i].flags.enable_robot = 0;
		}
	}
	PIR1bits.TMR1IF = 0;
}

