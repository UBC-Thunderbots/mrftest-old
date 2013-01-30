#ifndef PERCONFIG_H
#define PERCONFIG_H

/**
 * \file
 *
 * \brief Provides a union that allows memory to be reused between different USB configurations (which are necessarily mutually exclusive)
 */

#include "stdbool.h"
#include "stdint.h"

/**
 * \brief An outbound packet buffer used in normal mode
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
 * \brief An inbound packet buffer used in normal mode
 */
typedef struct normal_in_packet {
	struct normal_in_packet *next;
	uint8_t length;
	uint8_t data[103];
} normal_in_packet_t;

/**
 * \brief A packet buffer used in promiscuous mode
 */
typedef struct promisc_packet {
	struct promisc_packet *next;
	uint8_t length;
	uint8_t data[131];
} promisc_packet_t;

/**
 * \brief The reusable memory block
 */
extern union perconfig {
	struct {
		normal_out_packet_t out_packets[128];
		normal_in_packet_t in_packets[64];
		uint32_t drive_packet[64 / 4];
	} normal;
	promisc_packet_t promisc_packets[256];
} perconfig;

#endif

