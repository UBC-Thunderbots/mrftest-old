#ifndef XBEE_TXPACKET_H
#define XBEE_TXPACKET_H

/**
 * \file
 *
 * \brief Permits the sending of packets over a serial line to an XBee.
 */

#include <stdint.h>

/**
 * \brief A single contiguous sequence of bytes that can be gathered into a packet.
 */
typedef struct {
	/**
	 * \brief The length of this block, in bytes.
	 */
	uint8_t len;

	/**
	 * \brief The data to send.
	 */
	const uint8_t *ptr;
} xbee_txpacket_iovec_t;

/**
 * \brief A complete packet to transmit.
 */
typedef struct xbee_txpacket {
	/**
	 * \brief The number of iovecs in the iovs array.
	 */
	uint8_t num_iovs;

	/**
	 * \brief The iovecs defining the data making up the packet.
	 */
	__data xbee_txpacket_iovec_t *iovs;

	/**
	 * \brief The next packet in a linked list (for internal use only).
	 */
	__data struct xbee_txpacket *next;
} xbee_txpacket_t;

/**
 * \brief Initializes the serial ports and gets ready to transmit data.
 */
void xbee_txpacket_init(void);

/**
 * \brief Shuts down serial transmission.
 */
void xbee_txpacket_deinit(void);

/**
 * \brief Suspends serial transmission.
 *
 * This function waits for any in-transit bytes to be sent.
 * It does not necessarily suspend at a packet boundary.
 */
void xbee_txpacket_suspend(void);

/**
 * \brief Resumes suspended serial transmission.
 */
void xbee_txpacket_resume(void);

/**
 * \brief Queues a packet for transmission to an XBee.
 *
 * \param[in] packet the packet to queue.
 *
 * \param[in] xbee the index of the XBee to send to (0 or 1).
 */
void xbee_txpacket_queue(__data xbee_txpacket_t *packet, uint8_t xbee);

/**
 * \brief Retrieves a completed packet.
 *
 * \return an arbitrary completed packet, or null if no packets are in the done queue.
 */
__data xbee_txpacket_t *xbee_txpacket_dequeue(void);

#endif

