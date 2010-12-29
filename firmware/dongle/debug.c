#include "critsec.h"
#include "debug.h"
#include "usb.h"
#include <pic18fregs.h>

/**
 * \brief The endpoint number on which the debug output is reported.
 */
#define DEBUG_ENDPOINT 6

/**
 * \brief The bits structure of the UEP register for the debug output's endpoint.
 */
#define DEBUG_UEP_BITS UEP6bits

/**
 * \brief The buffer.
 */
static char buffer[64];

/**
 * \brief Whether or not the buffer is currently given to the SIE.
 */
static BOOL buffer_owned_by_sie;

volatile BOOL debug_enabled = false;

static void on_in(void) {
}

void debug_init(void) {
	debug_enabled = false;
	buffer_owned_by_sie = false;
	stdout = STREAM_USER;
	usb_bdpairs[DEBUG_ENDPOINT].in.BDCNT = 0;
	usb_bdpairs[DEBUG_ENDPOINT].in.BDADR = buffer;
	/* Enable timer 2 with a period of 1/12000000 × 256 × 16 × 4 =~ 1.37ms
	 *        /-------- Ignored
	 *         ////---- Postscale 1:16
	 *             /--- Timer 2 off
	 *              //- Prescale 1:4 */
	T2CON = 0b01111001;
	PR2 = 255;
}

void debug_enable(void) {
	buffer_owned_by_sie = false;
	usb_bdpairs[DEBUG_ENDPOINT].in.BDSTAT = 0;
	usb_bdpairs[DEBUG_ENDPOINT].in.BDCNT = 0;
	usb_ep_callbacks[DEBUG_ENDPOINT].in = &on_in;
	DEBUG_UEP_BITS.EPHSHK = 1;
	DEBUG_UEP_BITS.EPINEN = 1;
	debug_enabled = true;
}

void debug_disable(void) {
	debug_enabled = false;
	buffer_owned_by_sie = false;
	DEBUG_UEP_BITS.EPINEN = 0;
}

void debug_halt(void) {
	usb_bdpairs[DEBUG_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
}

void debug_unhalt(void) {
	usb_bdpairs[DEBUG_ENDPOINT].in.BDSTAT = 0;
}

PUTCHAR(ch) {
	BOOL send = false;
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);

	if (debug_enabled && !(usb_halted_in_endpoints & (1 << DEBUG_ENDPOINT))) {
		/* If the buffer has been submitted to the SIE, wait until it's finished.
		 * However, it's possible that this could cause problems:
		 * (1) If a SETUP transaction arrives, PKTDIS will be set and no further activity will occur.
		 * (2) If the transaction FIFO becomes full with other traffic, the debug endpoint cannot flush.
		 * (3) If the application fails to poll the debug endpoint, it will not flush.
		 * To account for these, we check PKTDIS and also set a timeout. */
		TMR2 = 0;
		PIR1bits.TMR2IF = 0;
		T2CONbits.TMR2ON = 1;
		while (usb_bdpairs[DEBUG_ENDPOINT].in.BDSTATbits.sie.UOWN) {
			if (PIR1bits.TMR1IF || UCONbits.PKTDIS) {
				CRITSEC_LEAVE(cs);
				T2CONbits.TMR2ON = 0;
				return;
			}
		}
		T2CONbits.TMR2ON = 0;

		/* If the buffer was previously owned by the SIE, reset the byte count and update the data toggle. */
		if (buffer_owned_by_sie) {
			buffer_owned_by_sie = false;
			usb_bdpairs[DEBUG_ENDPOINT].in.BDCNT = 0;
			usb_bdpairs[DEBUG_ENDPOINT].in.BDSTAT ^= BDSTAT_DTS;
		}

		if (ch == '\n') {
			send = true;
		} else {
			buffer[usb_bdpairs[DEBUG_ENDPOINT].in.BDCNT] = ch;
			if (++usb_bdpairs[DEBUG_ENDPOINT].in.BDCNT == 64) {
				send = true;
			}
		}

		if (send) {
			/* A newline marks end of message. Transmit whatever is in the buffer immediately, even if it's zero bytes. */
			if (usb_bdpairs[DEBUG_ENDPOINT].in.BDSTATbits.sie.OLDDTS) {
				usb_bdpairs[DEBUG_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN;
			} else {
				usb_bdpairs[DEBUG_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
			}
			buffer_owned_by_sie = true;
		}
	}

	CRITSEC_LEAVE(cs);
}

