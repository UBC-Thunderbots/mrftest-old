#ifndef MOTORS_H
#define MOTORS_H

/**
 * \file
 *
 * \brief Functions for operating motors.
 */

#include <stdint.h>

/**
 * \brief The possible methods of driving a motor.
 */
typedef enum {
	MANUAL_COMMUTATION,
	BRAKE,
	FORWARD,
	BACKWARD
} direction_t;

/**
 * \brief The patterns to use for manual commutation.
 */
extern uint8_t motor_manual_commutation_patterns[5];

/**
 * \brief Coasts all motors immediately.
 */
void motor_scram();

/**
 * \brief Sets the configuration of a wheel.
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
 * \brief Sets the configuration of the dribbler.
 *
 * \param[in] direction dribbler direction to set
 *
 * \param[in] pwm_level level to set dribbler speed
 */
void set_dribbler(direction_t direction, uint8_t pwm_level);

#endif

