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
 * \brief Checks if there is data to send and if the SIE is ready to accept new data.
 */
static void check_send(void) {
	/* See if there's a free BD to report on. */
	if (USB_BD_IN_HAS_FREE(EP_DONGLE_STATUS)) {
		if (memcmp(&back_buffer, &dongle_status, sizeof(back_buffer)) != 0) {
			/* Some status indicator actually changed. Queue for transmission. */
			memcpy(&back_buffer, &dongle_status, sizeof(back_buffer));
			USB_BD_IN_SUBMIT(EP_DONGLE_STATUS, &back_buffer, sizeof(back_buffer));
		}
	}
}

static void on_commanded_stall(void) {
	USB_BD_IN_COMMANDED_STALL(EP_DONGLE_STATUS);
}

static BOOL on_clear_halt(void) {
	USB_BD_IN_UNSTALL(EP_DONGLE_STATUS);
	check_send();
	return true;
}

void dongle_status_start(void) {
	usb_ep_callbacks[EP_DONGLE_STATUS].in.transaction = &check_send;
	usb_ep_callbacks[EP_DONGLE_STATUS].in.commanded_stall = &on_commanded_stall;
	usb_ep_callbacks[EP_DONGLE_STATUS].in.clear_halt = &on_clear_halt;
	USB_BD_IN_INIT(EP_DONGLE_STATUS);
	UEPBITS(EP_DONGLE_STATUS).EPHSHK = 1;
	UEPBITS(EP_DONGLE_STATUS).EPINEN = 1;
	reporting = true;
}

void dongle_status_stop(void) {
	reporting = false;
	UEPBITS(EP_DONGLE_STATUS).EPINEN = 0;
}

void dongle_status_dirty(void) {
	CRITSEC_DECLARE(cs);
	CRITSEC_ENTER(cs);
	if (reporting) {
		check_send();
	}
	CRITSEC_LEAVE(cs);
}

