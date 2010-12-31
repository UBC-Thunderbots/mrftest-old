#include "debug.h"
#include "critsec.h"
#include "endpoints.h"
#include "usb.h"
#include <pic18fregs.h>

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
	usb_bdpairs[EP_DEBUG].in.BDCNT = 0;
	usb_bdpairs[EP_DEBUG].in.BDADR = buffer;
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
	usb_bdpairs[EP_DEBUG].in.BDSTAT = 0;
	usb_bdpairs[EP_DEBUG].in.BDCNT = 0;
	usb_ep_callbacks[EP_DEBUG].in = &on_in;
	UEPBITS(EP_DEBUG).EPHSHK = 1;
	UEPBITS(EP_DEBUG).EPINEN = 1;
	debug_enabled = true;
}

void debug_disable(void) {
	debug_enabled = false;
	buffer_owned_by_sie = false;
	UEPBITS(EP_DEBUG).EPINEN = 0;
}

void debug_halt(void) {
	usb_bdpairs[EP_DEBUG].in.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
}

void debug_unhalt(void) {
	usb_bdpairs[EP_DEBUG].in.BDSTAT = 0;
}

PUTCHAR(ch) {
	BOOL send = false;
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);

	if (debug_enabled && !(usb_halted_in_endpoints & (1 << EP_DEBUG))) {
		/* If the buffer has been submitted to the SIE, wait until it's finished.
		 * However, it's possible that this could cause problems:
		 * (1) If a SETUP transaction arrives, PKTDIS will be set and no further activity will occur.
		 * (2) If the transaction FIFO becomes full with other traffic, the debug endpoint cannot flush.
		 * (3) If the application fails to poll the debug endpoint, it will not flush.
		 * To account for these, we check PKTDIS and also set a timeout. */
		TMR2 = 0;
		PIR1bits.TMR2IF = 0;
		T2CONbits.TMR2ON = 1;
		while (usb_bdpairs[EP_DEBUG].in.BDSTATbits.sie.UOWN) {
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
			usb_bdpairs[EP_DEBUG].in.BDCNT = 0;
			usb_bdpairs[EP_DEBUG].in.BDSTAT ^= BDSTAT_DTS;
		}

		if (ch == '\n') {
			send = true;
		} else {
			buffer[usb_bdpairs[EP_DEBUG].in.BDCNT] = ch;
			if (++usb_bdpairs[EP_DEBUG].in.BDCNT == 64) {
				send = true;
			}
		}

		if (send) {
			/* A newline marks end of message. Transmit whatever is in the buffer immediately, even if it's zero bytes. */
			if (usb_bdpairs[EP_DEBUG].in.BDSTATbits.sie.OLDDTS) {
				usb_bdpairs[EP_DEBUG].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN;
			} else {
				usb_bdpairs[EP_DEBUG].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
			}
			buffer_owned_by_sie = true;
		}
	}

	CRITSEC_LEAVE(cs);
}

