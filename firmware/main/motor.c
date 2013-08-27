#include "motor.h"
#include "io.h"

uint8_t motor_manual_commutation_patterns[5] = { 0, 0, 0, 0, 0 };
static uint16_t old_positions[5];

void motor_scram(void) {
	for (unsigned int index = 0; index < 5; ++index) {
		motor_manual_commutation_patterns[index] = 0;
		motor_set(index, MOTOR_MODE_MANUAL_COMMUTATION, 0);
	}
}

void motor_set(unsigned int wheel_num, motor_mode_t mode, uint8_t pwm_level) {
	// First, float all the pins so we don’t get glitches between adjusting mode and PWM level.
	IO_MOTOR(wheel_num).manual_commutation_pattern = 0;
	{
		io_motor_csr_t csr = { 0 };
		IO_MOTOR(wheel_num).csr = csr;
	}

	// No phases are connected to PWM, so we can now safely adjust the PWM duty cycle to its new value.
	IO_MOTOR(wheel_num).pwm = pwm_level;

	// Set the new mode, enabling phases appropriately.
	if (mode == MOTOR_MODE_MANUAL_COMMUTATION) {
		IO_MOTOR(wheel_num).manual_commutation_pattern = motor_manual_commutation_patterns[wheel_num];
	} else {
		io_motor_csr_t csr = { 0 };
		csr.mode =
			mode == MOTOR_MODE_BRAKE ? 1 :
			mode == MOTOR_MODE_FORWARD ? 2 :
			3;
		IO_MOTOR(wheel_num).csr = csr;
	}
}

int16_t motor_speed(unsigned int motor_num) {
	uint16_t position = IO_MOTOR(motor_num).position;
	int16_t speed = (int16_t) (uint16_t) (position - old_positions[motor_num]);
	old_positions[motor_num] = position;
	return speed;
}

uint8_t motor_sensor_failed(unsigned int motor_num) {
	io_motor_csr_t csr = IO_MOTOR(motor_num).csr;
	csr.clear_failures = 1;
	IO_MOTOR(motor_num).csr = csr;
	uint8_t ret = 0;
	if (csr.hall_stuck_low) {
		ret |= 1;
	}
	if (csr.hall_stuck_high) {
		ret |= 2;
	}
	if (csr.encoder_failed) {
		ret |= 4;
	}
	return ret;
}

