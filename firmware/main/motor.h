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

/**
 * \brief gets current pwm value of a motor
 *
 * \param[in] wheel_num wheel number to read current pwm value
 *
 * \return current pwm of selected wheel
 *
 * Just a helper function to allow for computed indexing
 */
uint8_t read_wheel_pwm(uint8_t wheel_num);

/**
 * \brief set wheel pwm value
 *
 * \param[in] wheel_num index of wheel to control
 *
 * \param[in] pwm_level pwm value to set
 *
 * Sets motor to specific pwm value (helper for computed indexing)
 */
void write_wheel_pwm(uint8_t wheel_num, uint8_t pwm_level);

#endif

