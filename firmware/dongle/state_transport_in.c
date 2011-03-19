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

__data uint8_t state_transport_in_feedback[16][sizeof(feedback_block_t)];

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
static uint8_t back_buffers[16][sizeof(feedback_block_t)];

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

	if (transaction_running) {
		return;
	}

	if (!USB_BD_IN_HAS_FREE(EP_STATE_TRANSPORT)) {
		return;
	}

	while (dirty_mask && blocks_used != BLOCKS_PER_TRANSACTION) {
		if (dirty_mask & (1 << next_robot)) {
			*write_ptr++ = sizeof(feedback_block_t) + 2;
			*write_ptr++ = (next_robot << 4) | PIPE_FEEDBACK;
			memcpyram2ram(write_ptr, back_buffers[next_robot], sizeof(feedback_block_t));
			write_ptr += sizeof(feedback_block_t);
			++blocks_used;
			dirty_mask &= ~(1 << next_robot);
		}
		next_robot = (next_robot + 1) & 15;
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

void state_transport_in_init(void) {
	/* The endpoint is held off until XBee stage 2 configuration completes. */
	usb_ep_callbacks[EP_STATE_TRANSPORT].in.transaction = &on_transaction;
	USB_BD_IN_INIT(EP_STATE_TRANSPORT);
	transaction_running = false;
	UEPBITS(EP_STATE_TRANSPORT).EPHSHK = 1;
	UEPBITS(EP_STATE_TRANSPORT).EPINEN = 1;
	dirty_mask = 0;
	next_robot = 0;
}

void state_transport_in_deinit(void) {
	UEPBITS(EP_STATE_TRANSPORT).EPINEN = 0;
}

void state_transport_in_feedback_dirty(uint8_t robot) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER_LOW(cs);

	/* Check if there's a real difference. */
	if (memcmp(back_buffers[robot], state_transport_in_feedback[robot], sizeof(back_buffers[robot])) != 0) {
		memcpyram2ram(back_buffers[robot], state_transport_in_feedback[robot], sizeof(back_buffers[robot]));
		dirty_mask |= 1 << robot;
		check_send();
	}

	CRITSEC_LEAVE(cs);
}

