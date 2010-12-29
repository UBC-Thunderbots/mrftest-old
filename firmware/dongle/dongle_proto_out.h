#ifndef DONGLE_PROTO_OUT_H
#define DONGLE_PROTO_OUT_H

/**
 * \file
 *
 * \brief Implements the dongle protocol (USB) side of outbound pipe transport.
 */

#include <stdint.h>

/**
 * \brief The type of a received micropacket.
 */
typedef struct dongle_proto_out_micropacket {
	/**
	 * \brief The micropacket's data, beginning with the length byte.
	 */
	__data uint8_t *ptr;

	/**
	 * \brief The index of the packet to which the micropacket belongs (internal use only).
	 */
	uint8_t packet;

	/**
	 * \brief The next micropacket structure in a linked list.
	 *
	 * This field can be used by the application when the micropacket is owned by the application.
	 */
	__data struct dongle_proto_out_micropacket *next;
} dongle_proto_out_micropacket_t;

/**
 * \brief Initializes the subsystem.
 */
void dongle_proto_out_init(void);

/**
 * \brief Deinitializes the subsystem.
 */
void dongle_proto_out_deinit(void);

/**
 * \brief Halts one of the endpoints.
 *
 * \param[in] ep the endpoint address to halt.
 */
void dongle_proto_out_halt(uint8_t ep);

/**
 * \brief Unhalts one of the endpoints.
 *
 * \param[in] ep the endpoint address to unhalt.
 */
void dongle_proto_out_unhalt(uint8_t ep);

/**
 * \brief Gets the next available micropacket.
 *
 * \return the micropacket, or null if no new micropackets are currently available.
 */
__data dongle_proto_out_micropacket_t *dongle_proto_out_get(void);

/**
 * \brief Releases a micropacket.
 *
 * \param[in] micropacket the micropacket to free.
 */
void dongle_proto_out_free(__data dongle_proto_out_micropacket_t *micropacket);

#endif

