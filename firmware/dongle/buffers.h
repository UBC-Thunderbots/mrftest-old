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

#define NUM_DONGLE_PROTO_OUT_INTERRUPT_BUFFERS 4

#define NUM_DONGLE_PROTO_OUT_BULK_BUFFERS 2

#define NUM_DONGLE_PROTO_OUT_BUFFERS (NUM_DONGLE_PROTO_OUT_INTERRUPT_BUFFERS + NUM_DONGLE_PROTO_OUT_BULK_BUFFERS)

/**
 * \brief Pointers to all the buffers suitable for USB transaction reception.
 */
extern __data void * __code const dongle_proto_out_buffers[NUM_DONGLE_PROTO_OUT_BUFFERS];

#endif

