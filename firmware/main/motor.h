#ifndef MOTOR_H
#define MOTOR_H

/**
 * \file
 *
 * \brief Functions for operating motors.
 */

#include <stdint.h>

/**
 * \brief The possible modes of driving a motor.
 */
typedef enum {
	MOTOR_MODE_MANUAL_COMMUTATION,
	MOTOR_MODE_BRAKE,
	MOTOR_MODE_FORWARD,
	MOTOR_MODE_BACKWARD
} motor_mode_t;

/**
 * \brief The patterns to use for manual commutation.
 */
extern uint8_t motor_manual_commutation_patterns[5];

/**
 * \brief Coasts all motors immediately.
 */
void motor_scram(void);

/**
 * \brief Drives a motor.
 *
 * \param[in] motor_num the motor number, 0–3 for a wheel or 4 for the dribbler
 *
 * \param[in] mode the mode in which to drive the motor
 *
 * \param[in] pwm_level the PWM duty cycle to send
 *
 */
void motor_set(uint8_t motor_num, motor_mode_t mode, uint8_t pwm_level);

#endif

