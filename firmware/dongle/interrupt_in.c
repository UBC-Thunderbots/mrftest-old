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

void interrupt_in_init(void) {
	usb_ep_callbacks[EP_INTERRUPT].in.transaction = &on_transaction;
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

