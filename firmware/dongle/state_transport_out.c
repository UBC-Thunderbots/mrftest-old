#include "state_transport_out.h"
#include "dongle_status.h"
#include "endpoints.h"
#include "error_reporting.h"
#include "pipes.h"
#include "signal.h"
#include "stackcheck.h"
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

__data drive_block_t state_transport_out_drive[16];

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

	stackcheck();

	/* Decode the packet. */
	while (left) {
		if (ptr[0] <= left) {
			switch (ptr[1] & 0x0F) {
				case PIPE_DRIVE:
					/* Drive pipe. */
					if (ptr[0] == sizeof(drive_block_t) + 2) {
						recipient = ptr[1] >> 4;
						memcpyram2ram(&state_transport_out_drive[recipient], ptr + 2, sizeof(drive_block_t));
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

	/* Prepare to receive the next packet. */
	USB_BD_OUT_SUBMIT(EP_STATE_TRANSPORT, buffer, sizeof(buffer));
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
	T1CON = 0b00110001;
	IPR1bits.TMR1IP = 0;
	PIE1bits.TMR1IE = 1;

	/* Start the endpoint. */
	usb_ep_callbacks[EP_STATE_TRANSPORT].out.transaction = &on_transaction;
	USB_BD_OUT_INIT(EP_STATE_TRANSPORT);
	USB_BD_OUT_SUBMIT(EP_STATE_TRANSPORT, buffer, sizeof(buffer));
	UEPBITS(EP_STATE_TRANSPORT).EPHSHK = 1;
	UEPBITS(EP_STATE_TRANSPORT).EPOUTEN = 1;
}

void state_transport_out_deinit(void) {
	UEPBITS(EP_STATE_TRANSPORT).EPOUTEN = 0;
}

SIGHANDLER(state_transport_out_tmr1if) {
	if (scram_timeout) {
		--scram_timeout;
	} else {
		uint8_t i;
		for (i = 0; i != 16; ++i) {
			state_transport_out_drive[i].flags.enable_robot = 0;
		}
	}
	PIR1bits.TMR1IF = 0;
}

