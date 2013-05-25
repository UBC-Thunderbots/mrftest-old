#include "motor.h"
#include "buffers.h"
#include "io.h"

uint8_t motor_manual_commutation_patterns[5] = { 0, 0, 0, 0, 0 };

void motor_scram() {
	for (uint8_t index = 0; index < 5; ++index) {
		motor_manual_commutation_patterns[index] = 0;
		set_wheel(index, MANUAL_COMMUTATION, 0);
	}
}

void set_wheel(uint8_t wheel_num, direction_t direction, uint8_t pwm_level) {
	if (wheel_num <= 4) {
		if (direction == BACKWARD) {
			current_buffer->tick.motor_directions |= 1 << wheel_num;
		} else {
			current_buffer->tick.motor_directions &= ~(1 << wheel_num);
		}
		if (direction == BRAKE) {
			current_buffer->tick.motor_drives[wheel_num] = 0;
		} else {
			current_buffer->tick.motor_drives[wheel_num] = pwm_level;
		}
		MOTOR_INDEX = wheel_num;
		if (direction == MANUAL_COMMUTATION) {
			MOTOR_CTL = motor_manual_commutation_patterns[wheel_num];
			MOTOR_PWM = pwm_level;
		} else if (direction == BRAKE) {
			MOTOR_CTL = 0b10101000;
			MOTOR_PWM = 0;
		} else {
			MOTOR_PWM = pwm_level;
			MOTOR_CTL = direction == BACKWARD ? 0b00000011 : 0b00000010;
		}
	}
}

void set_dribbler(direction_t direction, uint8_t pwm_level) {
	set_wheel(4, direction, pwm_level);
}

