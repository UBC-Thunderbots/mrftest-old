#include "bulk_out.h"
#include "critsec.h"
#include "dongle_status.h"
#include "endpoints.h"
#include "local_error_queue.h"
#include "stack.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <string.h>

STACK_DEFINE_TYPE(bulk_out_packet_t);

static bulk_out_packet_t packet1, packet2, packet3, packet4;

/**
 * \brief The packet buffers that are free.
 */
static STACK_TYPE(bulk_out_packet_t) free_packets;

/**
 * \brief The packet buffer currently submitted to the SIE.
 */
static __data bulk_out_packet_t *sie_packet;

/**
 * \brief The packets currently awaiting processing by the application.
 */
static QUEUE_TYPE(bulk_out_packet_t) ready_packets;

/**
 * \brief Whether or not a transaction is currently running.
 */
static volatile BOOL transaction_running;

static void submit_sie_packet(void) {
	/* If there are no free BDs, do nothing. */
	if (!USB_BD_OUT_HAS_FREE(EP_BULK)) {
		return;
	}

	/* If there is no packet, fetch one. */
	if (!sie_packet) {
		sie_packet = STACK_TOP(free_packets);
		STACK_POP(free_packets);
	}

	/* Submit a packet if we have one. */
	if (sie_packet) {
		USB_BD_OUT_SUBMIT(EP_BULK, sie_packet->buffer, sizeof(sie_packet->buffer));
		transaction_running = true;
	}
}

static void on_transaction(void) {
	/* Send the current packet to the ready queue. */
	sie_packet->cookie = 0;
	sie_packet->length = USB_BD_OUT_RECEIVED(EP_BULK);
	QUEUE_PUSH(ready_packets, sie_packet);
	sie_packet = 0;

	/* The transaction has finished. */
	transaction_running = false;

	/* Submit another packet to the SIE. */
	submit_sie_packet();
}

static void on_commanded_stall(void) {
	USB_BD_OUT_COMMANDED_STALL(EP_BULK);
	transaction_running = true;
}

static BOOL on_clear_halt(void) {
	/* Halt status can only be cleared once XBee stage 2 configuration completes. */
	if (dongle_status.xbees == XBEES_STATE_RUNNING) {
		USB_BD_OUT_UNSTALL(EP_BULK);
		transaction_running = false;
		submit_sie_packet();
		return true;
	} else {
		return false;
	}
}

void bulk_out_init(void) {
	/* Initialize queues. */
	STACK_INIT(free_packets);
	STACK_PUSH(free_packets, &packet1);
	STACK_PUSH(free_packets, &packet2);
	STACK_PUSH(free_packets, &packet3);
	STACK_PUSH(free_packets, &packet4);
	sie_packet = 0;
	QUEUE_INIT(ready_packets);

	/* The endpoint is halted until XBee stage 2 configuration completes. */
	usb_halted_out_endpoints |= 1 << EP_BULK;
	usb_ep_callbacks[EP_BULK].out.transaction = &on_transaction;
	usb_ep_callbacks[EP_BULK].out.commanded_stall = &on_commanded_stall;
	usb_ep_callbacks[EP_BULK].out.clear_halt = &on_clear_halt;
	USB_BD_OUT_INIT(EP_BULK);
	USB_BD_OUT_FUNCTIONAL_STALL(EP_BULK);
	transaction_running = true;
	UEPBITS(EP_BULK).EPHSHK = 1;
	UEPBITS(EP_BULK).EPOUTEN = 1;
}

void bulk_out_deinit(void) {
	UEPBITS(EP_BULK).EPOUTEN = 0;
}

__data bulk_out_packet_t *bulk_out_get(void) {
	__data bulk_out_packet_t *pkt;
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);
	pkt = QUEUE_FRONT(ready_packets);
	QUEUE_POP(ready_packets);
	CRITSEC_LEAVE(cs);
	return pkt;
}

void bulk_out_unget(__data bulk_out_packet_t *pkt) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);
	QUEUE_UNPOP(ready_packets, pkt);
	CRITSEC_LEAVE(cs);
}

void bulk_out_free(__data bulk_out_packet_t *pkt) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);
	STACK_PUSH(free_packets, pkt);
	if (!transaction_running) {
		submit_sie_packet();
	}
	CRITSEC_LEAVE(cs);
}

