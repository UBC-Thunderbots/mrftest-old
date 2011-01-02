#ifndef BULK_OUT_H
#define BULK_OUT_H

/**
 * \file
 *
 * \brief Implements the dongle protocol (USB) side of outbound bulk packet reception.
 */

#include "queue.h"
#include <stdint.h>

/**
 * \brief A bulk packet.
 */
typedef struct bulk_out_packet {
	/* TODO: This is a workaround for SDCC bug #3147475. */
	uint8_t dummy;

	/**
	 * \brief A byte the application can use for any purpose.
	 *
	 * This field is always set to 0 for a fresh packet,
	 * but keeps its value if a packet if a packet is requeued with interrupt_out_unget().
	 */
	uint8_t cookie;

	/**
	 * \brief The next packet in the linked list.
	 */
	__data struct bulk_out_packet *next;

	/**
	 * \brief The length of the packet.
	 */
	uint8_t length;

	/**
	 * \brief The packet data.
	 */
	uint8_t buffer[64];
} bulk_out_packet_t;

QUEUE_DEFINE_TYPE(bulk_out_packet_t);

/**
 * \brief Initializes the subsystem.
 */
void bulk_out_init(void);

/**
 * \brief Deinitializes the subsystem.
 */
void bulk_out_deinit(void);

/**
 * \brief Fetches a received packet.
 *
 * \return the packet, or null if no packets are pending.
 */
__data bulk_out_packet_t *bulk_out_get(void);

/**
 * \brief Pushes back a received packet so it will be fetched again.
 *
 * \param[in] pkt the packet to push back.
 */
void bulk_out_unget(__data bulk_out_packet_t *pkt);

/**
 * \brief Frees a packet buffer.
 *
 * \param[in] pkt a packet buffer returned by bulk_out_get().
 */
void bulk_out_free(__data bulk_out_packet_t *pkt);

#endif

