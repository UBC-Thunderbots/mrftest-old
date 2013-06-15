#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

/**
 * \brief Invoked once per tick when the control loop is not enabled to clear accumulated state.
 */
void control_clear(void);

/**
 * \brief Invoked when new setpoints are received from the host.
 *
 * This function should read the new setpoints and write them into \ref wheels_setpoints in an appropriate internal format.
 * This function must not assume anything about the value of \ref wheels_setpoints.
 *
 * \param setpoints the new setpoints
 */
void control_process_new_setpoints(const int16_t setpoints[4]);

/**
 * \brief Invoked once per tick when the control loop is enabled.
 *
 * This function should read from \ref wheels_setpoints, \ref wheels_encoder_counts, and the controller’s internal state, and write to \ref wheels_drives.
 * This function may also modify \ref wheels_setpoints.
 * This function may assume that the value it finds in \ref wheels_setpoints was placed there either by \ref control_process_new_setpoints or \ref control_tick.
 *
 * \param battery the battery voltage
 */
void control_tick(float battery);

#endif

