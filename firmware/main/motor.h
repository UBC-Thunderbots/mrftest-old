#ifndef MOTORS_H
#define MOTORS_H

/**
 * \file
 *
 * \brief file with motor helper functions
 */

#include <stdint.h>

/**
 * \brief possible motor control states
 */
typedef enum {
	FLOAT,
	BRAKE,
	FORWARD,
	BACKWARD
} direction_t;

/**
 * \brief motor safety switch
 *
 * sets all motor pwms to 0 and floats direction
 */
void motor_scram();

/**
 * \brief wheel safety switch
 */
void wheel_scram();

/**
 * \brief wheel control interface
 *
 * \param[in] wheel_num wheel index from 0
 *
 * \param[in] direction motor state to set to
 *
 * \param[in] pwm_level pwm level to set to
 *
 */
void set_wheel(uint8_t wheel_num, direction_t direction, uint8_t pwm_level);

/**
 * \brief dribbler safety switch
 */
void dribbler_scram();

/**
 * \brief set the dribbler direction and level
 *
 * \param[in] direction dribbler direction to set
 *
 * \param[in] pwm_level level to set dribbler speed
 */
void set_dribbler(direction_t direction, uint8_t pwm_level);

#endif

