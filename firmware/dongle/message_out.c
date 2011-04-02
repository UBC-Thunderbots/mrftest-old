#include "message_out.h"
#include "critsec.h"
#include "dongle_status.h"
#include "endpoints.h"
#include "error_reporting.h"
#include "pipes.h"
#include "stack.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <string.h>

STACK_DEFINE_TYPE(message_out_packet_t);

static message_out_packet_t packet1, packet2, packet3, packet4, packet5, packet6;

/**
 * \brief The packet buffers that are free.
 */
static STACK_TYPE(message_out_packet_t) free_packets;

/**
 * \brief The packet buffer currently submitted to the SIE.
 */
static __data message_out_packet_t *sie_packet;

/**
 * \brief The packets currently awaiting processing by the application.
 */
static QUEUE_TYPE(message_out_packet_t) ready_packets;

/**
 * \brief Whether or not a transaction is currently running.
 */
static volatile BOOL transaction_running;

static void submit_sie_packet(void) {
	uint8_t sz;

	/* If there are no free BDs, do nothing. */
	if (!USB_BD_OUT_HAS_FREE(EP_MESSAGE)) {
		return;
	}

	/* If there is no packet, fetch one. */
	if (!sie_packet) {
		sie_packet = STACK_TOP(free_packets);
		STACK_POP(free_packets);
		sie_packet->length = 0;
	}

	/* Submit a packet if we have one. */
	if (sie_packet) {
		sz = sizeof(sie_packet->buffer) - sie_packet->length;
		if (sz > 64) {
			sz = 64;
		}
		USB_BD_OUT_SUBMIT(EP_MESSAGE, sie_packet->buffer + sie_packet->length, sz);
		transaction_running = true;
	}
}

#include "pins.h"
static void on_transaction(void) {
	uint8_t robot, pipe;

	/* Remember that no transaction is running right now. */
	transaction_running = false;

	/* Record the number of bytes received. */
	sie_packet->length += USB_BD_OUT_RECEIVED(EP_MESSAGE);

	if (USB_BD_OUT_RECEIVED(EP_MESSAGE) == 64) {
		/* Full-length transaction means more transactions are needed. */
		submit_sie_packet();
		return;
	}

	/* Extract the header. */
	robot = sie_packet->buffer[0] >> 4;
	pipe = sie_packet->buffer[0] & 0x0F;
	if (pipe > PIPE_MAX || !((1 << pipe) & PIPE_OUT_MASK & PIPE_MESSAGE_MASK)) {
		/* Pipe is not an outbound message pipe.
		 * Report an error and leave the packet in sie_packet to resubmit. */
		error_reporting_add(FAULT_OUT_MICROPACKET_NOPIPE);
		sie_packet->length = 0;
	} else {
		/* Send the current packet to the ready queue. */
		sie_packet->cookie = 0xFF;
		QUEUE_PUSH(ready_packets, sie_packet);
		sie_packet = 0;
	}

	/* Submit another packet to the SIE. */
	submit_sie_packet();
}

void message_out_init(void) {
	/* Initialize queues. */
	STACK_INIT(free_packets);
	STACK_PUSH(free_packets, &packet1);
	STACK_PUSH(free_packets, &packet2);
	STACK_PUSH(free_packets, &packet3);
	STACK_PUSH(free_packets, &packet4);
	STACK_PUSH(free_packets, &packet5);
	STACK_PUSH(free_packets, &packet6);
	sie_packet = 0;
	QUEUE_INIT(ready_packets);

	/* Start the endpoint. */
	usb_ep_callbacks[EP_MESSAGE].out.transaction = &on_transaction;
	USB_BD_OUT_INIT(EP_MESSAGE);
	UEPBITS(EP_MESSAGE).EPHSHK = 1;
	UEPBITS(EP_MESSAGE).EPOUTEN = 1;
	transaction_running = false;
	submit_sie_packet();
}

void message_out_deinit(void) {
	UEPBITS(EP_MESSAGE).EPOUTEN = 0;
}

__data message_out_packet_t *message_out_get(void) {
	__data message_out_packet_t *pkt;
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER_LOW(cs);
	pkt = QUEUE_FRONT(ready_packets);
	QUEUE_POP(ready_packets);
	CRITSEC_LEAVE(cs);
	return pkt;
}

void message_out_unget(__data message_out_packet_t *pkt) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER_LOW(cs);
	QUEUE_UNPOP(ready_packets, pkt);
	CRITSEC_LEAVE(cs);
}

void message_out_free(__data message_out_packet_t *pkt) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER_LOW(cs);
	STACK_PUSH(free_packets, pkt);
	if (!transaction_running) {
		submit_sie_packet();
	}
	CRITSEC_LEAVE(cs);
}

