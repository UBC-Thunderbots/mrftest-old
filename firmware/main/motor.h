#ifndef MOTOR_H
#define MOTOR_H

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

void motor_init(void);
void motor_shutdown(void);
void motor_set(unsigned int motor_num, motor_mode_t mode, uint8_t pwm_level);
void motor_set_manual_commutation_pattern(unsigned int motor, uint8_t pattern);
void motor_force_power(void);
bool motor_hall_stuck_low(unsigned int motor);
bool motor_hall_stuck_high(unsigned int motor);
void motor_hall_stuck_clear(void);
void motor_tick(void);

#endif

