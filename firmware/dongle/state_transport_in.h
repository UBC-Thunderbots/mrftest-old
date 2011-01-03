#ifndef STATE_TRANSPORT_IN_H
#define STATE_TRANSPORT_IN_H

/**
 * \file
 *
 * \brief Implements the dongle protocol (USB) side of inbound state transport packet reception.
 */

#include <stdint.h>

/**
 * \brief The length, in bytes, of a state block on the feedback pipe.
 */
#define STATE_TRANSPORT_IN_FEEDBACK_SIZE 5

/**
 * \brief The current feedback pipe state blocks for the robots.
 */
__data extern uint8_t state_transport_in_feedback[15][STATE_TRANSPORT_IN_FEEDBACK_SIZE];

/**
 * \brief Initializes the subsystem.
 */
void state_transport_in_init(void);

/**
 * \brief Deinitializes the subsystem.
 */
void state_transport_in_deinit(void);

/**
 * \brief Marks a robot's feedback state block as being potentially dirty.
 *
 * \param[in] robot the index of the robot.
 */
void state_transport_in_feedback_dirty(uint8_t robot);

#endif

