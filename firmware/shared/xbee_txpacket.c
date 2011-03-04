#include "critsec.h"
#include "pins.h"
#include "queue.h"
#include "signal.h"
#include "stack.h"
#include "stackcheck.h"
#include "xbee_txpacket.h"
#include <pic18fregs.h>
#include <stdbool.h>

/**
 * \brief The states a transmitter can be in.
 */
typedef enum {
	/**
	 * \brief The transmitter will send a SOP when it can, if it has a packet to send.
	 */
	TXSTATE_SOP,

	/**
	 * \brief The transmitter will send the MSB of the packet length.
	 */
	TXSTATE_LENGTH_MSB,

	/**
	 * \brief The transmitter will send the LSB of the packet length, or an escape if appropriate.
	 */
	TXSTATE_LENGTH_LSB,

	/**
	 * \brief The transmitter will send a regular data byte, or an escape if appropriate.
	 */
	TXSTATE_DATA,

	/**
	 * \brief The transmitter will send a checksum byte, or an escape if appropriate.
	 */
	TXSTATE_CHECKSUM,
} txstate_t;

QUEUE_DEFINE_TYPE(xbee_txpacket_t);

STACK_DEFINE_TYPE(xbee_txpacket_t);

/**
 * \brief The states of the two transmitters.
 */
static txstate_t states[2];

/**
 * \brief The index of the current IOV in the current packet for each transmitter.
 */
static uint8_t iov_indices[2];

/**
 * \brief The index of the next byte to send in the current IOV in the current packet for each transmitter.
 */
static uint8_t byte_indices[2];

/**
 * \brief The packets waiting to be sent to each XBee.
 */
static QUEUE_TYPE(xbee_txpacket_t) ready_queue[2];

/**
 * \brief The packets that have been sent.
 */
static STACK_TYPE(xbee_txpacket_t) done_stack;

/**
 * \brief The accumulated checksums so far.
 */
static uint8_t checksums[2];

/**
 * \brief Whether or not the subsystem is initialized.
 */
static BOOL inited = false;

/**
 * \brief Iterates the iovecs in the current packet to update its length value.
 *
 * \param[in] xbee the index of the queue to examine.
 *
 * \return the length of the packet, in bytes.
 */
static uint8_t compute_current_packet_length(uint8_t xbee) {
	__data const xbee_txpacket_t *packet = QUEUE_FRONT(ready_queue[xbee]);
	uint8_t num_iovs;
	__data const xbee_txpacket_iovec_t *iovs;
	uint8_t answer = 0;
	if (packet) {
		num_iovs = packet->num_iovs;
		iovs = packet->iovs;
		while (num_iovs--) {
			answer += (iovs++)->len;
		}
	}
	return answer;
}

void xbee_txpacket_init(void) {
	if (!inited) {
		/* Turn on timer 4 with a period of 256 × 16 × 4 / 12000000 =~ 1.4ms.
		 *        /-------- Unimplemented
		 *        |////---- 1:16 postscale
		 *        |||||/--- Timer disabled (enabled only as needed)
		 *        ||||||//- 1:4 prescale */
		T4CON = 0b01111001;
		PR4 = 0xFF;

		/* Set timer 4 interrupts as low-priority, and enable them. */
		IPR3bits.TMR4IP = 0;
		PIR3bits.TMR4IF = 0;
		PIE3bits.TMR4IE = 1;

		/* Enable the transmitters. */
		TXSTA1bits.TXEN = 1;
		TXSTA2bits.TXEN = 1;

		/* Workaround for errata #3, must ensure that a 2-cycle instruction is not executed immediately after enabling a serial port. */
		Nop();
		Nop();

		/* Set transmit-ready interrupts to be low priority. Do not enable them yet as we have no data to send. */
		IPR1bits.TX1IP = 0;
		IPR3bits.TX2IP = 0;

		/* Initialize packet queues. */
		QUEUE_INIT(ready_queue[0]);
		QUEUE_INIT(ready_queue[1]);
		STACK_INIT(done_stack);

		/* Initialize transmitter states. */
		states[0] = states[1] = TXSTATE_SOP;
		iov_indices[0] = iov_indices[1] = byte_indices[0] = byte_indices[1] = 0;

		/* Remember that the module is initialized. */
		inited = true;
	}
}

void xbee_txpacket_deinit(void) {
	if (inited) {
		/* Disable the transmitters and interrupts. */
		PIE1bits.TX1IE = 0;
		PIE3bits.TX2IE = 0;
		TXSTA1bits.TXEN = 0;
		TXSTA2bits.TXEN = 0;

		/* Remember that the module is not initialized. */
		inited = false;
	}
}

void xbee_txpacket_suspend(void) {
	if (inited) {
		/* Disable interrupts. */
		PIE1bits.TX1IE = 0;
		PIE3bits.TX2IE = 0;

		/* Wait until both transmitters' shift registers are empty. */
		while (!TXSTA1bits.TRMT);
		while (!TXSTA2bits.TRMT);
	}
}

void xbee_txpacket_resume(void) {
	if (inited) {
		/* Re-enable interrupts. The ISRs are robust against the occasional spurious invocation. */
		PIE1bits.TX1IE = 1;
		PIE3bits.TX2IE = 1;
	}
}

void xbee_txpacket_queue(__data xbee_txpacket_t *packet, uint8_t xbee) {
	BOOL was_empty;
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER_LOW(cs);

	if (inited) {
		/* Add the packet to the end of the appropriate queue. */
		was_empty = !QUEUE_FRONT(ready_queue[xbee]);
		QUEUE_PUSH(ready_queue[xbee], packet);
		if (was_empty) {
			/* Enable transmit interrupts for the relevant USART. */
			if (xbee == 0) {
				PIE3bits.TX2IE = 1;
			} else /* if (xbee == 1) */ {
				PIE1bits.TX1IE = 1;
			}
		}
	}

	CRITSEC_LEAVE(cs);
}

__data xbee_txpacket_t *xbee_txpacket_dequeue(void) {
	__data xbee_txpacket_t *ret;
	CRITSEC_DECLARE(cs);

	CRITSEC_ENTER_LOW(cs);

	if (inited) {
		/* Remove head of queue, if any. */
		ret = STACK_TOP(done_stack);
		STACK_POP(done_stack);
	} else {
		ret = 0;
	}

	CRITSEC_LEAVE(cs);
	return ret;
}

/**
 * \brief A template for the code of a transmit interrupt handler.
 */
static inline void implement_txif(uint8_t xbeeidx) {
	__data const xbee_txpacket_iovec_t *iov;
	__data xbee_txpacket_t *packet;
	__data const uint8_t *ptr;
#define RD_TXIF() ((xbeeidx == 0) ? PIR3bits.TX2IF : PIR1bits.TX1IF)
#define CLR_TXIE() do { if (xbeeidx == 0) { PIE3bits.TX2IE = 0; } else { PIE1bits.TX1IE = 0; } } while (0)
#define WR_TXREG(val) do { if (xbeeidx == 0) { TXREG2 = val; } else { TXREG1 = val; } } while (0)
#define RD_CTS() ((xbeeidx == 0) ? PORT_XBEE0_CTS : PORT_XBEE1_CTS)

	stackcheck();

	/* If the shift register is empty, we may be able to send two bytes at once.
	 * Also, we may not actually send a byte if we shift a lot of state around.
	 * Thus, loop as long as we are able to transmit. */
	while (RD_TXIF()) {
		/* Grab the first packet on the queue. */
		packet = QUEUE_FRONT(ready_queue[xbeeidx]);

		/* If no packets are queued, disable interrupts and return. */
		if (!packet) {
			CLR_TXIE();
			return;
		}

		/* If the XBee has deasserted CTS (active-low), stop transmitting and enable the polling timer. */
		if (RD_CTS()) {
			CLR_TXIE();
			T4CONbits.TMR4ON = 1;
			return;
		}

		/* Do something useful based on current state. */
		switch (states[xbeeidx]) {
			case TXSTATE_SOP:
				/* Send a SOP. */
				WR_TXREG(0x7E);
				/* Initialize the checksum. */
				checksums[xbeeidx] = 0xFF;
				/* Advance the state machine. */
				states[xbeeidx] = TXSTATE_LENGTH_MSB;
				break;

			case TXSTATE_LENGTH_MSB:
				/* MSB of length is always zero. */
				WR_TXREG(0x00);
				/* Advance the state machine. */
				states[xbeeidx] = TXSTATE_LENGTH_LSB;
				break;

			case TXSTATE_LENGTH_LSB:
				/* Send byte. */
				WR_TXREG(compute_current_packet_length(xbeeidx));
				/* Advance the state machine. */
				states[xbeeidx] = TXSTATE_DATA;
				break;

			case TXSTATE_DATA:
				if (iov_indices[xbeeidx] == packet->num_iovs) {
					/* There are no iovecs left. Time to send the checksum. */
					states[xbeeidx] = TXSTATE_CHECKSUM;
					iov_indices[xbeeidx] = 0;
				} else {
					iov = packet->iovs + iov_indices[xbeeidx];
					if (byte_indices[xbeeidx] == iov->len) {
						/* There are no bytes left in the current iovec. Go to the next one. */
						++iov_indices[xbeeidx];
						byte_indices[xbeeidx] = 0;
					} else {
						/* We can send the next byte. */
						ptr = iov->ptr;
						ptr += byte_indices[xbeeidx];
						WR_TXREG(ptr[0]);
						checksums[xbeeidx] -= ptr[0];
						++byte_indices[xbeeidx];
					}
				}
				break;

			case TXSTATE_CHECKSUM:
				/* Send the checksum. */
				WR_TXREG(checksums[xbeeidx]);
				/* The packet is now finished. Send it to the done queue. */
				QUEUE_POP(ready_queue[xbeeidx]);
				STACK_PUSH(done_stack, packet);
				/* Next order of business will be to send the next packet's SOP. */
				states[xbeeidx] = TXSTATE_SOP;
				break;
		}

		/* TXnIF is not valid in the first cycle after a load of TXREG.
		 * Add a NOP here so the loop condition is guaranteed to be correct. */
		Nop();
	}
#undef RD_TXIF
#undef CLR_TXIE
#undef WR_TXREG
#undef RD_CTS
}

/*
IMPLEMENT_TXIF(1, 1, 1)
IMPLEMENT_TXIF(2, 0, 3)
*/
SIGHANDLER(xbee_txpacket_tx1if) {
	implement_txif(1);
}

SIGHANDLER(xbee_txpacket_tx2if) {
	implement_txif(0);
}

/**
 * \brief Handles TMR4 interrupts, which occur every 1ms.
 *
 * When a transmitter enters CTS holdoff, it will enable these interrupts.
 * At each interrupt, CTS is polled.
 * If CTS has been reasserted, the transmit interrupt is re-enabled.
 * If both CTS lines are asserted, the interrupt is disabled.
 */
SIGHANDLER(xbee_txpacket_tmr4if) {
	uint8_t cts_shadow = 0;

	/* Note: CTS lines are active-low. */
	if (!PORT_XBEE0_CTS) {
		cts_shadow |= 1;
	}
	if (!PORT_XBEE1_CTS) {
		cts_shadow |= 2;
	}
	if ((cts_shadow & 1) && QUEUE_FRONT(ready_queue[0])) {
		PIE3bits.TX2IE = 1;
	}
	if ((cts_shadow & 2) && QUEUE_FRONT(ready_queue[1])) {
		PIE1bits.TX1IE = 1;
	}
	if (cts_shadow == 3) {
		T4CONbits.TMR4ON = 0;
	}
	PIR3bits.TMR4IF = 0;
}

