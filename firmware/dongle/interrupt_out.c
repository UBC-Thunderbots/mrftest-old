#include "interrupt_out.h"
#include "critsec.h"
#include "dongle_status.h"
#include "endpoints.h"
#include "local_error_queue.h"
#include "pipes.h"
#include "stack.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <string.h>

STACK_DEFINE_TYPE(interrupt_out_packet_t);

static interrupt_out_packet_t packets[6];

/**
 * \brief The packet buffers that are free.
 */
static STACK_TYPE(interrupt_out_packet_t) free_packets;

/**
 * \brief The packet buffer currently submitted to the SIE.
 */
static __data interrupt_out_packet_t *sie_packet;

/**
 * \brief The packets currently awaiting processing by the application.
 */
static QUEUE_TYPE(interrupt_out_packet_t) ready_packets;

/**
 * \brief Whether or not a transaction is currently running.
 */
static volatile BOOL transaction_running;

static void submit_sie_packet(void) {
	/* If there are no free BDs, do nothing. */
	if (!USB_BD_OUT_HAS_FREE(EP_INTERRUPT)) {
		return;
	}

	/* If there is no packet, fetch one. */
	if (!sie_packet) {
		sie_packet = STACK_TOP(free_packets);
		STACK_POP(free_packets);
	}

	/* Submit a packet if we have one. */
	if (sie_packet) {
		USB_BD_OUT_SUBMIT(EP_INTERRUPT, sie_packet->buffer, sizeof(sie_packet->buffer));
		transaction_running = true;
	}
}

static void on_transaction(void) {
	uint8_t robot, pipe;

	/* Remember that no transaction is running right now. */
	transaction_running = false;

	if (USB_BD_OUT_RECEIVED(EP_INTERRUPT)) {
		/* Extract the header. */
		robot = sie_packet->buffer[0] >> 4;
		pipe = sie_packet->buffer[0] & 0x0F;
		if (!robot) {
			/* Robot index zero is invalid.
			 * Report an error and leave the packet in sie_packet to resubmit. */
			local_error_queue_add(38);
		} else if (pipe > PIPE_MAX || !((1 << pipe) & pipe_out_mask & pipe_interrupt_mask)) {
			/* Pipe is not an outbound interrupt pipe.
			 * Report an error and leave the packet in sie_packet to resubmit. */
			local_error_queue_add(38);
		} else {
			/* Send the current packet to the ready queue. */
			sie_packet->cookie = 0xFF;
			sie_packet->length = USB_BD_OUT_RECEIVED(EP_INTERRUPT);
			QUEUE_PUSH(ready_packets, sie_packet);
			sie_packet = 0;
		}
	} else {
		/* Zero-length packets are meaningless.
		 * Leave the packet in sie_packet to resubmit. */
	}

	/* Submit another packet to the SIE. */
	submit_sie_packet();
}

static void on_commanded_stall(void) {
	USB_BD_OUT_COMMANDED_STALL(EP_INTERRUPT);
	transaction_running = true;
}

static BOOL on_clear_halt(void) {
	/* Halt status can only be cleared once XBee stage 2 configuration completes. */
	if (dongle_status.xbees == XBEES_STATE_RUNNING) {
		USB_BD_OUT_UNSTALL(EP_INTERRUPT);
		transaction_running = false;
		submit_sie_packet();
		return true;
	} else {
		return false;
	}
}

void interrupt_out_init(void) {
	uint8_t i;

	/* Initialize queues. */
	STACK_INIT(free_packets);
	for (i = 0; i != sizeof(packets) / sizeof(*packets); ++i) {
		STACK_PUSH(free_packets, &packets[i]);
	}
	sie_packet = 0;
	QUEUE_INIT(ready_packets);

	/* The endpoint is halted until XBee stage 2 configuration completes. */
	usb_halted_out_endpoints |= 1 << EP_INTERRUPT;
	usb_ep_callbacks[EP_INTERRUPT].out.transaction = &on_transaction;
	usb_ep_callbacks[EP_INTERRUPT].out.commanded_stall = &on_commanded_stall;
	usb_ep_callbacks[EP_INTERRUPT].out.clear_halt = &on_clear_halt;
	USB_BD_OUT_INIT(EP_INTERRUPT);
	USB_BD_OUT_FUNCTIONAL_STALL(EP_INTERRUPT);
	transaction_running = true;
	UEPBITS(EP_INTERRUPT).EPHSHK = 1;
	UEPBITS(EP_INTERRUPT).EPOUTEN = 1;
}

void interrupt_out_deinit(void) {
	UEPBITS(EP_INTERRUPT).EPOUTEN = 0;
}

__data interrupt_out_packet_t *interrupt_out_get(void) {
	__data interrupt_out_packet_t *pkt;
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);
	pkt = QUEUE_FRONT(ready_packets);
	QUEUE_POP(ready_packets);
	CRITSEC_LEAVE(cs);
	return pkt;
}

void interrupt_out_unget(__data interrupt_out_packet_t *pkt) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);
	QUEUE_UNPOP(ready_packets, pkt);
	CRITSEC_LEAVE(cs);
}

void interrupt_out_free(__data interrupt_out_packet_t *pkt) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);
	STACK_PUSH(free_packets, pkt);
	if (!transaction_running) {
		submit_sie_packet();
	}
	CRITSEC_LEAVE(cs);
}

