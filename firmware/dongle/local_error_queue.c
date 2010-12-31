#include "local_error_queue.h"
#include "critsec.h"
#include "endpoints.h"
#include "usb.h"
#include <pic18fregs.h>

/**
 * \brief The error queue.
 */
static uint8_t queue[64];

/**
 * \brief The index of the first queue element currently being sent to the host.
 */
static volatile uint8_t read_ptr;

/**
 * \brief The index of the next position in the queue to write an element into.
 */
static volatile uint8_t write_ptr;

/**
 * \brief Whether or not the subsystem is initialized.
 */
static volatile BOOL inited = false;

/**
 * \brief Checks if there is data to send and if the SIE is ready to accept new data.
 */
static void check_send(void) {
	/* See if there's a free BD to report on. */
	if (USB_BD_IN_HAS_FREE(EP_LOCAL_ERROR_QUEUE)) {
		if (read_ptr != write_ptr) {
			/* Some errors are in the queue. Queue for transmission. */
			uint8_t length;
			if (read_ptr < write_ptr) {
				length = write_ptr - read_ptr;
			} else {
				length = sizeof(queue) - read_ptr;
			}
			USB_BD_IN_SUBMIT(EP_LOCAL_ERROR_QUEUE, queue + read_ptr, length);
		}
	}
}

static void on_transaction(void) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);

	/* Advance the buffer pointer by the number of bytes transmitted. */
	read_ptr = (read_ptr + USB_BD_IN_SENT(EP_LOCAL_ERROR_QUEUE)) & (sizeof(queue) - 1);

	/* See if we have more to send. */
	check_send();

	CRITSEC_LEAVE(cs);
}

static void on_commanded_stall(void) {
	USB_BD_IN_COMMANDED_STALL(EP_LOCAL_ERROR_QUEUE);
}

static BOOL on_clear_halt(void) {
	USB_BD_IN_UNSTALL(EP_LOCAL_ERROR_QUEUE);
	check_send();
	return true;
}

void local_error_queue_init(void) {
	read_ptr = write_ptr = 0;
	usb_ep_callbacks[EP_LOCAL_ERROR_QUEUE].in.transaction = &on_transaction;
	usb_ep_callbacks[EP_LOCAL_ERROR_QUEUE].in.commanded_stall = &on_commanded_stall;
	usb_ep_callbacks[EP_LOCAL_ERROR_QUEUE].in.clear_halt = &on_clear_halt;
	USB_BD_IN_INIT(EP_LOCAL_ERROR_QUEUE);
	UEPBITS(EP_LOCAL_ERROR_QUEUE).EPHSHK = 1;
	UEPBITS(EP_LOCAL_ERROR_QUEUE).EPINEN = 1;
	inited = true;
}

void local_error_queue_deinit(void) {
	inited = false;
	UEPBITS(EP_LOCAL_ERROR_QUEUE).EPINEN = 0;
}

void local_error_queue_add(uint8_t error) {
	uint8_t free_space;
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);

	if (inited) {
		free_space = (read_ptr - write_ptr - 1) & (sizeof(queue) - 1);
		if (free_space) {
			if (free_space == 1) {
				/* This is the last slot in the error queue.
				 * Change the error we're about to encode to "Local error queue overflow". */
				error = 0;
			}
			queue[write_ptr] = error;
			write_ptr = (write_ptr + 1) & (sizeof(queue) - 1);
			check_send();
		}
	}

	CRITSEC_LEAVE(cs);
}

