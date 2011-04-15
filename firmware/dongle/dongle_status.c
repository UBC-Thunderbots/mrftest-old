#include "dongle_status.h"
#include "critsec.h"
#include "endpoints.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <string.h>

volatile dongle_status_t dongle_status = {
	ESTOP_STATE_UNINITIALIZED,
	XBEES_STATE_PREINIT,
	0,
};

/**
 * \brief The status block queued for sending over USB.
 */
static dongle_status_t back_buffer;

/**
 * \brief Whether or not status reporting is running.
 */
static BOOL reporting = false;

/**
 * \brief Whether a transmission should be forced even though nothing changed.
 */
static BOOL force;

/**
 * \brief Checks if there is data to send and if the SIE is ready to accept new data.
 */
static void check_send(void) {
	/* See if there's a free BD to report on. */
	if (USB_BD_IN_HAS_FREE(EP_DONGLE_STATUS)) {
		if (force || memcmp(&back_buffer, &dongle_status, sizeof(back_buffer))) {
			/* Some status indicator actually changed. Queue for transmission. */
			memcpy(&back_buffer, &dongle_status, sizeof(back_buffer));
			USB_BD_IN_SUBMIT(EP_DONGLE_STATUS, &back_buffer, sizeof(back_buffer));
			force = false;
		}
	}
}

void dongle_status_start(void) {
	dongle_status.estop = ESTOP_STATE_UNINITIALIZED;
	dongle_status.xbees = XBEES_STATE_PREINIT;
	dongle_status.robots = 0;
	usb_ep_callbacks[EP_DONGLE_STATUS].in.transaction = &check_send;
	USB_BD_IN_INIT(EP_DONGLE_STATUS);
	UEPBITS(EP_DONGLE_STATUS).EPHSHK = 1;
	UEPBITS(EP_DONGLE_STATUS).EPINEN = 1;
	reporting = true;
	force = true;
	check_send();
}

void dongle_status_stop(void) {
	reporting = false;
	UEPBITS(EP_DONGLE_STATUS).EPINEN = 0;
}

void dongle_status_dirty(void) {
	CRITSEC_DECLARE(cs);
	CRITSEC_ENTER_LOW(cs);
	if (reporting) {
		check_send();
	}
	CRITSEC_LEAVE(cs);
}

void dongle_status_force(void) {
	force = true;
	dongle_status_dirty();
}

