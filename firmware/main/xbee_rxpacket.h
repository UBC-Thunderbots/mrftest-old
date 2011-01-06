#ifndef XBEE_RXPACKET_H
#define XBEE_RXPACKET_H

/**
 * \file
 *
 * \brief Receives packets from the XBees and presents them to the main loop.
 */

#include <stdint.h>

/**
 * \brief A block of memory into which a packet can be received.
 */
typedef struct xbee_rxpacket {
	/**
	 * \brief The index of the XBee that sent this packet (0 or 1).
	 */
	uint8_t xbee;

	/**
	 * \brief The length of the packet.
	 *
	 * This field need not be set to the size of the buffer when the packet is submitted.
	 */
	uint8_t len;

	/**
	 * \brief The buffer.
	 */
	uint8_t buf[111];

	/**
	 * \brief The next packet in a linked list (for internal use only).
	 */
	__data struct xbee_rxpacket *next;
} xbee_rxpacket_t;

/**
 * \brief Initializes the serial ports and starts waiting for data.
 */
void xbee_rxpacket_init(void);

/**
 * \brief Shuts down serial reception.
 */
void xbee_rxpacket_deinit(void);

/**
 * \brief Suspends serial reception.
 *
 * This function deasserts RTS then waits for any in-transit bytes to be received.
 * It does not necessarily suspend at a packet boundary.
 * If an XBee does not honour the RTS line, data may be lost.
 */
void xbee_rxpacket_suspend(void);

/**
 * \brief Resumes suspended serial reception.
 */
void xbee_rxpacket_resume(void);

/**
 * \brief Retrieves a received packet.
 *
 * \return the next received packet, or null if no packets are available.
 */
__data xbee_rxpacket_t *xbee_rxpacket_get(void);

/**
 * \brief Returns a received packet buffer for reuse.
 *
 * \param[out] packet the packet buffer to free.
 */
void xbee_rxpacket_free(__data xbee_rxpacket_t *packet);

#endif

