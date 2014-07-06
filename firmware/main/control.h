#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

#define CONTROL_LOOP_HZ 200U

/**
 * \brief Invoked once per tick when the control loop is not enabled to clear accumulated state.
 */
void control_clear(void);

/**
 * \brief Updates the internal state of the controller when new setpoints are received from the host.
 *
 * \param[in] setpoints the new setpoints
 */
void control_process_new_setpoints(const int16_t setpoints[4U]);

/**
 * \brief Updates the internal state of the controller as time passes and computes new output drive levels.
 *
 * \param[in] feedback the current measured speeds of the wheels
 * \param[out] drive the drive levels to send to the motors
 */
void control_tick(const int16_t feedback[4U], int16_t drive[4U]);

#endif

