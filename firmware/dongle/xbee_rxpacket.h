#ifndef XBEE_RXPACKET_H
#define XBEE_RXPACKET_H

/**
 * \file
 *
 * \brief Receives packets from the XBees and presents them to the main loop.
 */

#include <stdint.h>

/**
 * \brief The possible errors that can occur while receiving data from XBees.
 */
typedef enum {
	/**
	 * \brief A byte framing error (bad stop bit) occurred.
	 */
	XBEE_RXPACKET_ERROR_FERR,

	/**
	 * \brief A hardware overrun error occurred.
	 *
	 * A hardware overrun error occurs when bytes arrive at the serial port faster than the interrupt service routine can remove them.
	 */
	XBEE_RXPACKET_ERROR_OERR_HW,

	/**
	 * \brief A software overrun error occurred.
	 *
	 * A software overrun error occurs when the XBee refuses to stop sending despite an RTS holdoff,
	 * and the interrupt service routine has no packet buffers available to hold the incoming packet.
	 */
	XBEE_RXPACKET_ERROR_OERR_SW,

	/**
	 * \brief A packet was received, but the checksum failed.
	 */
	XBEE_RXPACKET_ERROR_CHECKSUM_ERROR,

	/**
	 * \brief A packet header was received with a nonzero value for the MSB of packet length.
	 */
	XBEE_RXPACKET_ERROR_LENGTH_MSB_NONZERO,

	/**
	 * \brief A packet header was received with an illegal value for the LSB of packet length.
	 */
	XBEE_RXPACKET_ERROR_LENGTH_LSB_ILLEGAL,
} xbee_rxpacket_error_t;

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
	 * \brief The buffer, which must be at least 111 bytes long.
	 */
	__data uint8_t *ptr;

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

