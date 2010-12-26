#include "local_error_queue.h"
#include "critsec.h"
#include "usb.h"
#include <pic18fregs.h>

/**
 * \brief The endpoint number on which the local error queue is reported.
 */
#define LOCAL_ERROR_QUEUE_ENDPOINT 2

/**
 * \brief The UEP register for the local error queue's endpoint.
 */
#define LOCAL_ERROR_QUEUE_UEP UEP2

/**
 * \brief The bits structure of the UEP register for the local error queue's endpoint.
 */
#define LOCAL_ERROR_QUEUE_UEP_BITS UEP2bits

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
	/* Only queue a USB packet if the subsystem is enabled. */
	if (inited) {
		/* The BD needs to be owned by the CPU. */
		if (!usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDSTATbits.cpu.UOWN) {
			if (usb_halted_in_endpoints & (1 << LOCAL_ERROR_QUEUE_ENDPOINT)) {
				/* The endpoint was halted by the host. */
				usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
			} else if (read_ptr != write_ptr) {
				/* Some errors are in the queue. Queue for transmission. */
				if (read_ptr < write_ptr) {
					usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDCNT = write_ptr - read_ptr;
				} else {
					usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDCNT = sizeof(queue) - read_ptr;
				}
				usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDADR = queue + read_ptr;
				if (usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDSTATbits.sie.OLDDTS) {
					usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTSEN;
				} else {
					usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_DTS | BDSTAT_DTSEN;
				}
			}
		}
	}
}

/**
 * \brief Handles completed transactions.
 */
static void on_in(void) {
	/* Advance the buffer pointer by the number of bytes transmitted. */
	read_ptr = (read_ptr + usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDCNT) & (sizeof(queue) - 1);
	usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDCNT = 0;

	/* See if we have more to send. */
	check_send();
}

void local_error_queue_init(void) {
	read_ptr = write_ptr = 0;
	usb_ep_callbacks[LOCAL_ERROR_QUEUE_ENDPOINT].in = &on_in;
	usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDSTAT = BDSTAT_DTS;
	LOCAL_ERROR_QUEUE_UEP = 0;
	LOCAL_ERROR_QUEUE_UEP_BITS.EPHSHK = 1;
	LOCAL_ERROR_QUEUE_UEP_BITS.EPINEN = 1;
	LOCAL_ERROR_QUEUE_UEP_BITS.EPCONDIS = 1;
	inited = true;
}

void local_error_queue_deinit(void) {
	inited = false;
	LOCAL_ERROR_QUEUE_UEP = 0;
	usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDSTAT = 0;
	usb_ep_callbacks[LOCAL_ERROR_QUEUE_ENDPOINT].in = 0;
}

void local_error_queue_halt(void) {
	usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDSTAT = BDSTAT_UOWN | BDSTAT_BSTALL;
	usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDCNT = 0;
}

void local_error_queue_unhalt(void) {
	usb_bdpairs[LOCAL_ERROR_QUEUE_ENDPOINT].in.BDSTAT = BDSTAT_DTS;
	check_send();
}

void local_error_queue_add(uint8_t error) {
	uint8_t free_space;

	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);

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

	CRITSEC_LEAVE(cs);
}

