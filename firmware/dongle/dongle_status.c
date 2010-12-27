#include "critsec.h"
#include "dongle_status.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <string.h>

/**
 * \brief The endpoint number on which the dongle status block is reported.
 */
#define DONGLE_STATUS_ENDPOINT 1

/**
 * \brief The UEP register for the dongle status block's endpoint.
 */
#define DONGLE_STATUS_UEP UEP1

/**
 * \brief The bits structure of the UEP register for the dongle status block's endpoint.
 */
#define DONGLE_STATUS_UEP_BITS UEP1bits

volatile dongle_status_t dongle_status = {
	ESTOP_STATE_UNINITIALIZED,
	XBEES_STATE_PREINIT,
	0,
};

/**
 * \brief The status block queued for sending over USB.
 */
static volatile dongle_status_t back_buffer;

/**
 * \brief Whether or not status reporting is running.
 */
static BOOL reporting = false;

/**
 * \brief Checks if there is data to send and if the SIE is ready to accept new data.
 */
static void check_send(void) {
	/* Only queue a USB packet if reporting is enabled. */
	if (reporting) {
		/* The BD needs to be owned by the CPU. */
		if (!usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDSTATbits.cpu.UOWN) {
			if (usb_halted_in_endpoints & (1 << DONGLE_STATUS_ENDPOINT)) {
				/* The endpoint was halted by the host. */
				usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
			} else if (memcmp(&back_buffer, &dongle_status, sizeof(back_buffer)) != 0) {
				/* Some status indicator actually changed. Queue for transmission. */
				memcpy(&back_buffer, &dongle_status, sizeof(back_buffer));
				usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDADR = &back_buffer;
				usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDCNT = sizeof(back_buffer);
				if (usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDSTATbits.sie.OLDDTS) {
					usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
				} else {
					usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN;
				}
			}
		}
	}
}

void dongle_status_start(void) {
	usb_ep_callbacks[DONGLE_STATUS_ENDPOINT].in = &check_send;
	usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDSTAT = BDSTAT_DTS;
	DONGLE_STATUS_UEP = 0;
	DONGLE_STATUS_UEP_BITS.EPHSHK = 1;
	DONGLE_STATUS_UEP_BITS.EPINEN = 1;
	DONGLE_STATUS_UEP_BITS.EPCONDIS = 1;
	reporting = true;
}

void dongle_status_stop(void) {
	reporting = false;
	DONGLE_STATUS_UEP = 0;
	usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDSTAT = 0;
	usb_ep_callbacks[DONGLE_STATUS_ENDPOINT].in = 0;
}

void dongle_status_halt(void) {
	usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
}

void dongle_status_unhalt(void) {
	usb_bdpairs[DONGLE_STATUS_ENDPOINT].in.BDSTAT = BDSTAT_DTS;
	check_send();
}

void dongle_status_dirty(void) {
	CRITSEC_DECLARE(cs);
	CRITSEC_ENTER(cs);
	check_send();
	CRITSEC_LEAVE(cs);
}

