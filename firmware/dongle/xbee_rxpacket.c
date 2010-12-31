#include "xbee_rxpacket.h"
#include "buffers.h"
#include "critsec.h"
#include "local_error_queue.h"
#include "pins.h"
#include "queue.h"
#include "signal.h"
#include "stack.h"
#include <pic18fregs.h>
#include <stdbool.h>

/**
 * \brief The possible low-level states of a single packet reception.
 */
typedef enum {
	/**
	 * \brief The receiver is waiting for a start-of-packet.
	 */
	STATE_EXPECT_SOP,

	/**
	 * \brief The receiver is waiting for the MSB of the payload length word.
	 */
	STATE_EXPECT_LENGTH_MSB,

	/**
	 * \brief The receiver is waiting for the LSB of the payload length word.
	 */
	STATE_EXPECT_LENGTH_LSB,

	/**
	 * \brief The receiver is waiting for a byte of payload.
	 */
	STATE_EXPECT_DATA,

	/**
	 * \brief The receiver is waiting for the packet checksum.
	 */
	STATE_EXPECT_CHECKSUM,

	/**
	 * \brief The receiver is ignoring bytes due to a prior software overrun error.
	 */
	STATE_IGNORING_BYTES,
} rxstate_state_t;

/**
 * \brief The complete data associated with an ongoing reception.
 */
typedef struct {
	/**
	 * \brief The current packet being received.
	 *
	 * Sometimes null in STATE_EXPECT_SOP, STATE_IGONRING_BYTES, STATE_EXPECT_LENGTH_MSB, or STATE_EXPECT_LENGTH_LSB.
	 * Never null in other states.
	 */
	__data xbee_rxpacket_t *packet;

	/**
	 * \brief The current state of the XBee.
	 */
	rxstate_state_t state;

	/**
	 * \brief The length of the current packet (in STATE_EXPECT_DATA) or the number of bytes to ignore (in STATE_IGNORING_BYTES).
	 */
	uint8_t length;

	/**
	 * \brief The number of bytes received so far (in STATE_EXPECT_DATA).
	 */
	uint8_t bytes_received;

	/**
	 * \brief The accumulated checksum.
	 */
	uint8_t checksum;
} rxstate_t;

QUEUE_DEFINE_TYPE(xbee_rxpacket_t);

STACK_DEFINE_TYPE(xbee_rxpacket_t);

/**
 * \brief The receive state blocks for the two XBees.
 */
static rxstate_t rxstates[2];

/**
 * \brief The empty packet buffers that have been provided to the subsystem but not yet received into.
 */
static STACK_TYPE(xbee_rxpacket_t) pending_stack;

/**
 * \brief The full buffers that are waiting to be given to the application.
 */
static QUEUE_TYPE(xbee_rxpacket_t) done_queue;

/**
 * \brief Packet structures suitable for XBee packet reception.
 */
static __data xbee_rxpacket_t xbee_packets[NUM_XBEE_BUFFERS];

/**
 * \brief Whether or not the subsystem is initialized.
 */
static BOOL inited = false;

void xbee_rxpacket_init(void) {
	uint8_t i;

	if (!inited) {
		/* Enable the receivers. */
		RCSTA1bits.CREN = 1;
		RCSTA2bits.CREN = 1;

		/* Workaround for errata #3, must ensure that a 2-cycle instruction is not executed immediately after enabling a serial port. */
		Nop();
		Nop();

		/* Initialize receiver states. */
		rxstates[0].packet = 0;
		rxstates[0].state = STATE_EXPECT_SOP;
		rxstates[1].packet = 0;
		rxstates[1].state = STATE_EXPECT_SOP;

		/* Initialize packet queues. */
		STACK_INIT(pending_stack);
		QUEUE_INIT(done_queue);
		for (i = 0; i < NUM_XBEE_BUFFERS; ++i) {
			xbee_packets[i].ptr = xbee_buffers[i];
			STACK_PUSH(pending_stack, &xbee_packets[i]);
		}

		/* Remember that the module is initialized. */
		inited = true;

		/* Set byte-received interrupts to be high priority and enable them. */
		IPR1bits.RC1IP = 1;
		IPR3bits.RC2IP = 1;
		PIE1bits.RC1IE = 1;
		PIE3bits.RC2IE = 1;

		/* Deassert RTS to the XBees. */
		LAT_XBEE0_RTS = 1;
		LAT_XBEE1_RTS = 1;
	}
}

void xbee_rxpacket_deinit(void) {
	if (inited) {
		/* Remember that the module is deinitialized. */
		inited = false;

		/* Disable the receivers. */
		PIE1bits.RC1IE = 0;
		PIE3bits.RC2IE = 0;
		RCSTA1bits.CREN = 0;
		RCSTA2bits.CREN = 0;

		/* Deassert RTS to the XBees. */
		LAT_XBEE0_RTS = 1;
		LAT_XBEE1_RTS = 1;

		/* Flush any outstanding interrupts. */
		while (PIR1bits.RC1IF) {
			(void) RCREG1;
		}
		while (PIR3bits.RC2IF) {
			(void) RCREG2;
		}
	}
}

void xbee_rxpacket_suspend(void) {
	if (inited) {
		/* Deassert both RTS lines. */
		LAT_XBEE0_RTS = 1;
		LAT_XBEE1_RTS = 1;

		/* Wait until no data is in transit.
		 * Do it twice because maybe there's an edge condition if a byte is just starting. */
		while (!BAUDCON1bits.RCIDL);
		while (!BAUDCON2bits.RCIDL);
		while (!BAUDCON1bits.RCIDL);
		while (!BAUDCON2bits.RCIDL);
	}
}

void xbee_rxpacket_resume(void) {
	if (inited) {
		/* Assert RTS only if we actually have a packet buffer available. */
		if (STACK_TOP(pending_stack)) {
			LAT_XBEE0_RTS = 0;
			LAT_XBEE1_RTS = 0;
		}
	}
}

__data xbee_rxpacket_t *xbee_rxpacket_get(void) {
	__data xbee_rxpacket_t *result = 0;
	CRITSEC_DECLARE(cs);

	if (QUEUE_FRONT(done_queue)) {
		CRITSEC_ENTER(cs);
		if (inited) {
			result = QUEUE_FRONT(done_queue);
			QUEUE_POP(done_queue);
		}
		CRITSEC_LEAVE(cs);
	}

	return result;
}

void xbee_rxpacket_free(__data xbee_rxpacket_t *packet) {
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER(cs);

	if (inited) {
		if (rxstates[0].state != STATE_EXPECT_SOP && !rxstates[0].packet) {
			/* XBee 0 needs a buffer right now for an in-progress packet.
			 * Give it the new buffer. */
			rxstates[0].packet = packet;
			LAT_XBEE0_RTS = 0;
		} else if (rxstates[1].state != STATE_EXPECT_SOP && !rxstates[1].packet) {
			/* XBee 1 needs a buffer right now for an in-progress packet.
			 * Give it the new buffer. */
			rxstates[1].packet = packet;
			LAT_XBEE1_RTS = 0;
		} else {
			/* Neither XBee is in the middle of receiving right now.
			 * Put the buffer in the queue and let either XBee use it. */
			STACK_PUSH(pending_stack, packet);
			LAT_XBEE0_RTS = 0;
			LAT_XBEE1_RTS = 0;
		}
	}

	CRITSEC_LEAVE(cs);
}

#define IMPLEMENT_RCIF(usartidx, xbeeidx, piridx) \
SIGHANDLER(xbee_rxpacket_rc ## usartidx ## if) { \
	uint8_t ch; \
\
	/* Check for hardware overrun error. */ \
	if (RCSTA ## usartidx ## bits.OERR) { \
		/* Overrun error can only be cleared by rebooting the module. */ \
		RCSTA ## usartidx ## bits.CREN = 0; \
		RCSTA ## usartidx ## bits.CREN = 1; \
		/* Record the error. */ \
		local_error_queue_add(3 + xbeeidx); \
		/* There's no way to retrieve the lost bytes, so the only thing to do is throw away this packet. */ \
		rxstates[xbeeidx].state = STATE_EXPECT_SOP; \
		return; \
	} \
\
	/* Check for framing error. */ \
	if (RCSTA ## usartidx ## bits.FERR) { \
		/* Framing error is cleared by receiving a legitimate byte afterwards, which we do below. */ \
		/* Record the error. */ \
		local_error_queue_add(1 + xbeeidx); \
		/* There's no way to retrieve the lost byte, so the only thing to do is throw away this packet. */ \
		rxstates[xbeeidx].state = STATE_EXPECT_SOP; \
	} \
\
	/* Handle the received byte. */ \
	ch = RCREG ## usartidx; \
	switch (rxstates[xbeeidx].state) { \
		case STATE_EXPECT_SOP: \
			if (ch == 0x7E) { \
				rxstates[xbeeidx].state = STATE_EXPECT_LENGTH_MSB; \
				if (rxstates[xbeeidx].packet) { \
					/* Packet buffer already allocated to this XBee. Just use it. */ \
				} else if (STACK_TOP(pending_stack)) { \
					/* Packet buffer available; allocate it. */ \
					rxstates[xbeeidx].packet = STACK_TOP(pending_stack); \
					rxstates[xbeeidx].packet->xbee = xbeeidx; \
					STACK_POP(pending_stack); \
				} else { \
					/* No packet buffers available. Make the XBee hold off. */ \
					LAT_XBEE ## xbeeidx ## _RTS = 1; \
				} \
			} \
			break; \
\
		case STATE_EXPECT_LENGTH_MSB: \
			/* The MSB of the length should always be zero; no packet should be >255B. */ \
			if (ch == 0) { \
				rxstates[xbeeidx].state = STATE_EXPECT_LENGTH_LSB; \
			} else { \
				/* Record the error. */ \
				local_error_queue_add(9 + xbeeidx); \
				/* There's no way to know how long the packet should be, so just go back to looking for SOP. */ \
				rxstates[xbeeidx].state = STATE_EXPECT_SOP; \
			} \
			break; \
\
		case STATE_EXPECT_LENGTH_LSB: \
			if (ch < 1 || ch > 111) { \
				/* Packets can't be of lengths outside this range.
				 * Report the error. */ \
				local_error_queue_add(11 + xbeeidx); \
				/* There's no way to know how long the packet should be, so just go back to looking for SOP. */ \
				rxstates[xbeeidx].state = STATE_EXPECT_SOP; \
			} else if (!rxstates[xbeeidx].packet) { \
				/* There's no packet buffer available.
				 * The XBee must be ignoring RTS holdoff.
				 * Report the error. */ \
				local_error_queue_add(5 + xbeeidx); \
				/* We can at least ignore the right number of bytes to avoid desynchronization. */ \
				rxstates[xbeeidx].length = ch; \
				rxstates[xbeeidx].state = STATE_IGNORING_BYTES; \
			} else { \
				/* Set up to receive the packet payload. */ \
				rxstates[xbeeidx].length = rxstates[xbeeidx].packet->len = ch; \
				rxstates[xbeeidx].bytes_received = 0; \
				rxstates[xbeeidx].checksum = 0; \
				rxstates[xbeeidx].state = STATE_EXPECT_DATA; \
			} \
			break; \
\
		case STATE_EXPECT_DATA: \
			/* Accumulate the byte into the packet buffer. */ \
			rxstates[xbeeidx].packet->ptr[rxstates[xbeeidx].bytes_received++] = ch; \
			/* Update the checksum. */ \
			rxstates[xbeeidx].checksum += ch; \
			if (rxstates[xbeeidx].bytes_received == rxstates[xbeeidx].length) { \
				/* There's no more payload left. The next byte should be the checksum. */ \
				rxstates[xbeeidx].state = STATE_EXPECT_CHECKSUM; \
			} \
			break; \
\
		case STATE_EXPECT_CHECKSUM: \
			/* Update the checksum. */ \
			rxstates[xbeeidx].checksum += ch; \
			if (rxstates[xbeeidx].checksum == 0xFF) { \
				/* The checksum is correct.
				 * Queue the packet for the application. */ \
				QUEUE_PUSH(done_queue, rxstates[xbeeidx].packet); \
				rxstates[xbeeidx].packet = 0; \
			} else { \
				/* The checksum is incorrect.
				 * Record the error. */ \
				local_error_queue_add(7 + xbeeidx); \
			} \
			/* The packet is over, so look for a SOP. */ \
			rxstates[xbeeidx].state = STATE_EXPECT_SOP; \
			break; \
\
		case STATE_IGNORING_BYTES: \
			if (!rxstates[xbeeidx].length) { \
				/* We've ignored as many bytes as we should.
				 * Go back to waiting for a SOP. */ \
				rxstates[xbeeidx].state = STATE_EXPECT_SOP; \
			} else { \
				rxstates[xbeeidx].length--; \
			} \
			break; \
	} \
}

IMPLEMENT_RCIF(1, 1, 1)
IMPLEMENT_RCIF(2, 0, 3)

