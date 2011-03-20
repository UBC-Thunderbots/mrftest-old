#ifndef STATE_TRANSPORT_OUT_H
#define STATE_TRANSPORT_OUT_H

/**
 * \file
 *
 * \brief Implements the dongle protocol (USB) side of outbound state transport packet reception.
 */

#include "drive.h"
#include <stdint.h>

/**
 * \brief The current drive pipe state blocks for the robots.
 */
__data extern uint8_t state_transport_out_drive[16][DRIVE_SIZE];

/**
 * \brief Initializes the subsystem.
 */
void state_transport_out_init(void);

/**
 * \brief Deinitializes the subsystem.
 */
void state_transport_out_deinit(void);

#endif

