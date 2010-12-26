#ifndef BUFFERS_H
#define BUFFERS_H

/**
 * \file
 *
 * \brief Defines blocks of memory to be used as buffers for receiving data.
 */

#include "xbee_rxpacket.h"

#define NUM_XBEE_BUFFERS 6

/**
 * \brief Pointers to all the buffers suitable for XBee packet reception.
 */
extern __data void * __code const xbee_buffers[NUM_XBEE_BUFFERS];

/**
 * \brief Packet structures suitable for XBee packet reception.
 */
extern __data xbee_rxpacket_t xbee_packets[NUM_XBEE_BUFFERS];

#endif

