#ifndef INTERRUPT_OUT_H
#define INTERRUPT_OUT_H

/**
 * \file
 *
 * \brief Implements the dongle protocol (USB) side of outbound interrupt packet reception.
 */

#include "queue.h"
#include <stdint.h>

/**
 * \brief A interrupt packet.
 */
typedef struct interrupt_out_packet {
	/* TODO: This is a workaround for SDCC bug #3147475. */
	uint8_t dummy;

	/**
	 * \brief A byte the application can use for any purpose.
	 *
	 * This field is always set to 0xFF for a fresh packet,
	 * but keeps its value if a packet if a packet is requeued with interrupt_out_unget().
	 */
	uint8_t cookie;

	/**
	 * \brief The next packet in the linked list.
	 */
	__data struct interrupt_out_packet *next;

	/**
	 * \brief The length of the packet.
	 */
	uint8_t length;

	/**
	 * \brief The packet data.
	 */
	uint8_t buffer[64];
} interrupt_out_packet_t;

QUEUE_DEFINE_TYPE(interrupt_out_packet_t);

/**
 * \brief Initializes the subsystem.
 */
void interrupt_out_init(void);

/**
 * \brief Deinitializes the subsystem.
 */
void interrupt_out_deinit(void);

/**
 * \brief Fetches a received packet.
 *
 * \return the packet, or null if no packets are pending.
 */
__data interrupt_out_packet_t *interrupt_out_get(void);

/**
 * \brief Pushes back a received packet so it will be fetched again.
 *
 * \param[in] pkt the packet to push back.
 */
void interrupt_out_unget(__data interrupt_out_packet_t *pkt);

/**
 * \brief Frees a packet buffer.
 *
 * \param[in] pkt a packet buffer returned by interrupt_out_get().
 */
void interrupt_out_free(__data interrupt_out_packet_t *pkt);

#endif

