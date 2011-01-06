#include "run.h"
#include "drive.h"
#include "feedback.h"
#include "params.h"
#include "pipes.h"
#include "xbee_rxpacket.h"
#include "xbee_txpacket.h"
#include <pic18fregs.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * \brief The XBee API ID for a transmit data packet.
 */
#define XBEE_API_ID_TX_16 0x01

/**
 * \brief The XBee API ID for a transmit status packet.
 */
#define XBEE_API_ID_TX_STATUS 0x89

/**
 * \brief The XBee API ID for a receive data packet.
 */
#define XBEE_API_ID_RX_16 0x81

/**
 * \brief The type of an XBee transmit header.
 */
typedef struct {
	uint8_t api_id;
	uint8_t frame_id;
	uint8_t address_high;
	uint8_t address_low;
	uint8_t options;
} xbee_tx16_header_t;

/**
 * \brief The possible states we can be in with respect to an inbound packet we are trying to send.
 */
typedef enum {
	/**
	 * \brief We are not trying to do anything with an inbound packet (any packet we might have sent has been delivered successfully).
	 */
	INBOUND_STATE_IDLE,

	/**
	 * \brief We are waiting for the inbound packet to be sent over the serial line to the XBee.
	 */
	INBOUND_STATE_SENDING,

	/**
	 * \brief We are waiting for XBee #1 to send us a transmit status packet.
	 */
	INBOUND_STATE_AWAITING_STATUS,

	/**
	 * \brief We are waiting for another poll because the packet could not be delivered and we need to try again.
	 */
	INBOUND_STATE_AWAITING_POLL,
} inbound_state_t;

void run(void) {
	__data xbee_rxpacket_t *rxpacket;
	__data const uint8_t *rxptr;
	static xbee_tx16_header_t txheader = { XBEE_API_ID_TX_16, 0x00, 0x7B, 0x30, 0x00 };
	static xbee_txpacket_iovec_t txiovs[12], txiovs_shadow[12];
	static xbee_txpacket_t txpkt, txpkt_shadow;
	BOOL txpkt_pending = false;
	inbound_state_t inbound_state = INBOUND_STATE_IDLE;
	static feedback_block_t txpkt_feedback_shadow;
	static uint8_t sequence[PIPE_MAX + 1];

	/* Clear state. */
	memset(sequence, 0, sizeof(sequence));
	txiovs[0].len = sizeof(txheader);
	txiovs[0].ptr = &txheader;
	txiovs[1].len = sizeof(txpkt_feedback_shadow);
	txiovs[1].ptr = &txpkt_feedback_shadow;
	txpkt.iovs = txiovs;
	txpkt.num_iovs = 2;

	/* Run forever handling events. */
	for (;;) {
		if (xbee_txpacket_dequeue() == &txpkt_shadow) {
			/* An inbound finished being sent. */
			if (inbound_state == INBOUND_STATE_SENDING) {
				/* Update our current state. */
				inbound_state = INBOUND_STATE_AWAITING_STATUS;

				/* Start a timeout to deal with possible serial line failures. */
				TMR0H = 0;
				TMR0L = 0;
				INTCONbits.TMR0IF = 0;
			}
		}

		if (rxpacket = xbee_rxpacket_get()) {
			/* There's an XBee packet to deal with. */
			if (rxpacket->xbee == 0 && rxpacket->len > 5 && rxpacket->buf[0] == XBEE_API_ID_RX_16 && rxpacket->buf[1] == 0x7B && rxpacket->buf[2] == 0x20) {
				/* It's a receive data packet from XBee #0 with data from the dongle. */
				if (rxpacket->buf[4] & 0x02) {
					/* It's a broadcast packet and therefore contains a poll code and a list of state transport micropackets for multiple robots. */
					rxptr = rxpacket->buf + 1;
					while (rxptr - rxpacket->buf < rxpacket->len) {
						if (rxptr[0] >= 2 && rxptr[0] <= (rxpacket->len - (rxptr - rxpacket->buf))) {
							/* It's reasonably correctly structured. */
							uint8_t robot = rxptr[1] >> 4;
							uint8_t pipe = rxptr[1] & 0x0F;
							if (robot && robot == params.robot_number) {
								/* It's addressed to us. */
								if (pipe == PIPE_DRIVE && rxptr[0] == 2 + sizeof(drive_block)) {
									/* It's new drive data. */
									memcpyram2ram(&drive_block, rxptr + 2, sizeof(drive_block));
								} else {
									/* It's addressed to an unknown pipe or is the wrong length. */
#warning report an error
								}
							}
							rxptr += rxptr[0];
						} else {
							/* It's structurally broken. */
#warning report an error
						}
					}

					if (rxpacket->buf[0] && rxpacket->buf[0] == params.robot_number && (inbound_state == INBOUND_STATE_IDLE || inbound_state == INBOUND_STATE_AWAITING_POLL)) {
						BOOL need_status = false;

						/* The poll code is asking us to send a packet. */
						if (txpkt_pending) {
							/* We should resend the last packet.
							 * The master copy should still be intact, so just shadow it and send it. */
							memcpyram2ram(txiovs_shadow, txiovs, sizeof(txiovs_shadow));
							memcpyram2ram(&txpkt_shadow, &txpkt, sizeof(txpkt_shadow));
							/* We do, however, want a new frame number. */
							if (!++txheader.frame_id) {
								txheader.frame_id = 1;
							}
							xbee_txpacket_queue(&txpkt_shadow, 1);
						} else {
							/* Shadow the feedback block to avoid byte tearing if it's updated shortly. */
							memcpyram2ram(&txpkt_feedback_shadow, &feedback_block, sizeof(txpkt_feedback_shadow));
							/* The rest of the application should have gradually pushed data into the IOVs.
							 * Thus, the packet should be completely ready to send right now.
							 * Note that the packet transmission layer will make a mess of our packet and IOVs, so shadow them. */
							memcpyram2ram(txiovs_shadow, txiovs, sizeof(txiovs_shadow));
							memcpyram2ram(&txpkt_shadow, &txpkt, sizeof(txpkt_shadow));
							xbee_txpacket_queue(&txpkt_shadow, 1);
						}

						/* Mark state. */
						inbound_state = INBOUND_STATE_SENDING;
					}
				} else if (rxpacket->len == 6 && rxpacket->buf[5] == 0xFF) {
					/* It's a discovery and synchronization packet.
					 * Clear our sequence numbers. */
					memset(sequence, 0, sizeof(sequence));
				} else {
					/* It's a packet containing a single interrupt or bulk message. */
#warning implement
				}
			} else if (rxpacket->xbee == 1 && rxpacket->len == 3 && rxpacket->buf[0] == XBEE_API_ID_TX_STATUS && rxpacket->buf[1] == txheader.frame_id && inbound_state == INBOUND_STATE_AWAITING_STATUS) {
				/* It's a transmit status packet from XBee #1 indicating the result of a transmission. */
				if (rxpacket->buf[2] == 0) {
					/* Transmission was successful.
					 * Clean up the IOVs and mark state. */
					txpkt.num_iovs = 2;
					inbound_state = INBOUND_STATE_IDLE;
				} else {
					/* Tranmission failed.
					 * Leave the packet in place and wait for the next polling time. */
					inbound_state = INBOUND_STATE_AWAITING_POLL;
				}
			}

			xbee_rxpacket_free(rxpacket);
		}

		if (INTCONbits.TMR0IF && inbound_state == INBOUND_STATE_AWAITING_STATUS) {
			/* Timeout waiting for transmit status, assume the serial line corrupted our data. */
#warning report erorr
		}
	}
}

