#ifndef BUFFERS_H
#define BUFFERS_H

/**
 * \file
 *
 * \brief Defines blocks of memory to be used as buffers for receiving data.
 */

#define NUM_XBEE_BUFFERS 6

/**
 * \brief Pointers to all the buffers suitable for XBee packet reception.
 */
extern __data void * __code const xbee_buffers[NUM_XBEE_BUFFERS];

#endif

