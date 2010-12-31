#ifndef DONGLE_PROTO_OUT_H
#define DONGLE_PROTO_OUT_H

/**
 * \file
 *
 * \brief Implements the dongle protocol (USB) side of outbound state transport packet reception.
 */

#include <stdint.h>

/**
 * \brief The length, in bytes, of a state block on the drive pipe.
 */
#define STATE_TRANSPORT_OUT_DRIVE_SIZE 9

/**
 * \brief The current drive pipe state blocks for the robots.
 */
__data extern uint8_t state_transport_out_drive[15][STATE_TRANSPORT_OUT_DRIVE_SIZE];

/**
 * \brief Initializes the subsystem.
 */
void state_transport_out_init(void);

/**
 * \brief Deinitializes the subsystem.
 */
void state_transport_out_deinit(void);

#endif

