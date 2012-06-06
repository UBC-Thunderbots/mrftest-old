#ifndef PERCONFIG_H
#define PERCONFIG_H

/**
 * \file
 *
 * \brief Provides a union that allows memory to be reused between different USB configurations (which are necessarily mutually exclusive)
 */

#include "stdint.h"

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
	promisc_packet_t promisc_packets[256];
} perconfig;

#endif

