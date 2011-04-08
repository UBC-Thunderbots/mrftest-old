#ifndef MESSAGE_OUT_H
#define MESSAGE_OUT_H

/**
 * \file
 *
 * \brief Implements the dongle protocol (USB) side of outbound message packet reception.
 */

#include "queue.h"
#include <stdint.h>

/**
 * \brief A message packet.
 */
typedef struct message_out_packet {
	/* TODO: This is a workaround for SDCC bug #3147475. */
	uint8_t dummy;

	/**
	 * \brief A byte the application can use for any purpose.
	 *
	 * This field is always set to 0xFF for a fresh packet,
	 * but keeps its value if a packet if a packet is requeued with message_out_unget().
	 */
	uint8_t cookie;

	/**
	 * \brief The next packet in the linked list.
	 */
	__data struct message_out_packet *next;

	/**
	 * \brief The length of the packet.
	 */
	uint8_t length;

	/**
	 * \brief The packet data.
	 */
	uint8_t buffer[64];
} message_out_packet_t;

QUEUE_DEFINE_TYPE(message_out_packet_t);

/**
 * \brief Initializes the subsystem.
 */
void message_out_init(void);

/**
 * \brief Deinitializes the subsystem.
 */
void message_out_deinit(void);

/**
 * \brief Fetches a received packet.
 *
 * \return the packet, or null if no packets are pending.
 */
__data message_out_packet_t *message_out_get(void);

/**
 * \brief Pushes back a received packet so it will be fetched again.
 *
 * \param[in] pkt the packet to push back.
 */
void message_out_unget(__data message_out_packet_t *pkt);

/**
 * \brief Frees a packet buffer.
 *
 * \param[in] pkt a packet buffer returned by message_out_get().
 */
void message_out_free(__data message_out_packet_t *pkt);

#endif

