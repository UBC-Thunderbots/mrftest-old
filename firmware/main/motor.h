#ifndef MOTOR_H
#define MOTOR_H

/**
 * \file
 *
 * \brief Functions for operating motors.
 */

#include "io.h"
#include <stdbool.h>
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
 * \brief Initializes the motor subsystem.
 */
void motor_init(void);

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
void motor_set(unsigned int motor_num, motor_mode_t mode, uint8_t pwm_level);

/**
 * \brief Returns the distance the motor has turned since this function was last called.
 *
 * For wheels, this is an optical encoder reading; for the dribbler, a Hall sensor reading.
 *
 * \param[in] motor_num the motor number, 0–3 for a wheel or 4 for the dribbler
 *
 * \return the motor speed
 */
int16_t motor_speed(unsigned int motor_num);

/**
 * \brief Checks for sensor failures in a motor.
 *
 * Any failures reported are cleared.
 *
 * \param[in] motor_num the motor number, 0–3 for a wheel or 4 for the dribbler
 *
 * \return a bitmask, with bit 0 indicating Hall sensors stuck low, bit 1 indicating Hall sensors stuck high, and bit 2 indicating encoder not commutating
 */
uint8_t motor_sensor_failed(unsigned int motor_num);

#endif

