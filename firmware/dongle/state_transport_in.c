#include "state_transport_in.h"
#include "critsec.h"
#include "dongle_status.h"
#include "endpoints.h"
#include "pipes.h"
#include "usb.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <string.h>

/**
 * \brief The number of feedback blocks to send in a single USB transaction.
 */
#define BLOCKS_PER_TRANSACTION (64 / (sizeof(feedback_block_t) + 2))

__data uint8_t state_transport_in_feedback[15][sizeof(feedback_block_t)];

/**
 * \brief A buffer into which packets are assembled for transmission.
 */
static uint8_t buffer[64];

/**
 * \brief A bitmask indicating which buffers are dirty.
 */
static uint16_t dirty_mask;

/**
 * \brief Back buffers containing old copies of the feedback blocks for comparison.
 */
static uint8_t back_buffers[15][sizeof(feedback_block_t)];

/**
 * \brief Whether or not there is a transaction currently running.
 */
static BOOL transaction_running;

/**
 * \brief The next robot whose block should be sent.
 */
static uint8_t next_robot;

static void check_send(void) {
	uint8_t blocks_used = 0;
	__data uint8_t *write_ptr = buffer;

	if (!USB_BD_IN_HAS_FREE(EP_STATE_TRANSPORT)) {
		return;
	}

	while (dirty_mask && blocks_used != BLOCKS_PER_TRANSACTION) {
		if (dirty_mask & (1 << next_robot)) {
			*write_ptr++ = sizeof(feedback_block_t) + 2;
			*write_ptr++ = (next_robot << 4) | PIPE_FEEDBACK;
			memcpyram2ram(write_ptr, back_buffers[next_robot - 1], sizeof(feedback_block_t));
			write_ptr += sizeof(feedback_block_t);
			++blocks_used;
			dirty_mask &= ~(1 << next_robot);
		}
		if (next_robot == 15) {
			next_robot = 1;
		} else {
			++next_robot;
		}
	}

	if (blocks_used) {
		USB_BD_IN_SUBMIT(EP_STATE_TRANSPORT, buffer, blocks_used * (sizeof(feedback_block_t) + 2));
		transaction_running = true;
	}
}

static void on_transaction(void) {
	transaction_running = false;
	check_send();
}

static void on_commanded_stall(void) {
	USB_BD_IN_COMMANDED_STALL(EP_STATE_TRANSPORT);
	transaction_running = true;
}

static BOOL on_clear_halt(void) {
	/* Halt status can only be cleared once XBee stage 2 configuration completes. */
	if (dongle_status.xbees == XBEES_STATE_RUNNING) {
		USB_BD_IN_UNSTALL(EP_STATE_TRANSPORT);
		transaction_running = false;
		dirty_mask = 0xFFFE;
		memcpyram2ram(back_buffers, state_transport_in_feedback, sizeof(back_buffers));
		check_send();
		return true;
	} else {
		return false;
	}
}

void state_transport_in_init(void) {
	/* The endpoint is halted until XBee stage 2 configuration completes. */
	usb_halted_in_endpoints |= 1 << EP_STATE_TRANSPORT;
	usb_ep_callbacks[EP_STATE_TRANSPORT].in.transaction = &on_transaction;
	usb_ep_callbacks[EP_STATE_TRANSPORT].in.commanded_stall = &on_commanded_stall;
	usb_ep_callbacks[EP_STATE_TRANSPORT].in.clear_halt = &on_clear_halt;
	USB_BD_IN_INIT(EP_STATE_TRANSPORT);
	USB_BD_IN_FUNCTIONAL_STALL(EP_STATE_TRANSPORT);
	transaction_running = true;
	UEPBITS(EP_STATE_TRANSPORT).EPHSHK = 1;
	UEPBITS(EP_STATE_TRANSPORT).EPINEN = 1;
	next_robot = 1;
}

void state_transport_in_deinit(void) {
	UEPBITS(EP_STATE_TRANSPORT).EPINEN = 0;
}

void state_transport_in_feedback_dirty(uint8_t robot) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER_LOW(cs);

	/* Check if there's a real difference. */
	if (memcmp(back_buffers[robot - 1], state_transport_in_feedback[robot - 1], sizeof(back_buffers[robot - 1])) != 0) {
		memcpyram2ram(back_buffers[robot - 1], state_transport_in_feedback[robot - 1], sizeof(back_buffers[robot - 1]));
		dirty_mask |= 1 << robot;
		if (!transaction_running) {
			check_send();
		}
	}

	CRITSEC_LEAVE(cs);
}

