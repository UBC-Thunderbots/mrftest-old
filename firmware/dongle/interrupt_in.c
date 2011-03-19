#include "interrupt_in.h"
#include "critsec.h"
#include "dongle_status.h"
#include "endpoints.h"
#include "global.h"
#include "pipes.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <string.h>

/**
 * \brief Whether or not there is a transaction currently running.
 */
static volatile BOOL busy;

static void on_transaction(void) {
	busy = false;
}

static void on_commanded_stall(void) {
	USB_BD_IN_COMMANDED_STALL(EP_INTERRUPT);
	busy = true;
}

static BOOL on_clear_halt(void) {
	USB_BD_IN_UNSTALL(EP_INTERRUPT);
	busy = false;
	return true;
}

void interrupt_in_init(void) {
	usb_ep_callbacks[EP_INTERRUPT].in.transaction = &on_transaction;
	usb_ep_callbacks[EP_INTERRUPT].in.commanded_stall = &on_commanded_stall;
	usb_ep_callbacks[EP_INTERRUPT].in.clear_halt = &on_clear_halt;
	USB_BD_IN_INIT(EP_INTERRUPT);
	busy = false;
	UEPBITS(EP_INTERRUPT).EPHSHK = 1;
	UEPBITS(EP_INTERRUPT).EPINEN = 1;
}

void interrupt_in_deinit(void) {
	UEPBITS(EP_INTERRUPT).EPINEN = 0;
}

void interrupt_in_send(__data const uint8_t *message, uint8_t len) {
	/* Wait until the endpoint is not busy. */
	while (busy) {
		if (should_shut_down) {
			return;
		}
	}

	/* Begin the transaction. */
	busy = true;
	USB_BD_IN_SUBMIT(EP_INTERRUPT, message, len);

	/* Wait until transmission is complete. */
	while (busy) {
		if (should_shut_down) {
			return;
		}
	}
}

