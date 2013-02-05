#ifndef PERCONFIG_H
#define PERCONFIG_H

/**
 * \file
 *
 * \brief Provides a union that allows memory to be reused between different USB configurations (which are necessarily mutually exclusive).
 */

#include "stdbool.h"
#include "stdint.h"

/**
 * \brief An outbound packet buffer used in normal mode (configuration 2).
 */
typedef struct normal_out_packet {
	struct normal_out_packet *next;
	bool reliable;
	uint8_t dest;
	uint8_t message_id;
	uint8_t delivery_status;
	uint8_t length;
	uint8_t retries_remaining;
	uint8_t data[64];
} normal_out_packet_t;

/**
 * \brief An inbound packet buffer used in normal mode (configuration 2).
 */
typedef struct normal_in_packet {
	struct normal_in_packet *next;
	uint8_t length;
	uint8_t data[103];
} normal_in_packet_t;

/**
 * \brief The complete collection of data used in normal mode (configuration 2).
 */
typedef struct {
	normal_out_packet_t out_packets[128];
	normal_in_packet_t in_packets[64];
	uint8_t drive_packet[64];
} normal_perconfig_t;

/**
 * \brief A packet buffer used in promiscuous mode.
 */
typedef struct promisc_packet {
	struct promisc_packet *next;
	uint8_t length;
	uint8_t data[131];
} promisc_packet_t;

/**
 * \brief The type of the reusable memory block.
 */
typedef union {
	normal_perconfig_t normal;
	promisc_packet_t promisc_packets[256];
} perconfig_t;

/**
 * \brief The actual memory block.
 */
extern perconfig_t perconfig;

#endif

