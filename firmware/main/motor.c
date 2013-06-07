#include "motor.h"
#include "io.h"

uint8_t motor_manual_commutation_patterns[5] = { 0, 0, 0, 0, 0 };

void motor_scram(void) {
	for (uint8_t index = 0; index < 5; ++index) {
		motor_manual_commutation_patterns[index] = 0;
		motor_set_wheel(index, MOTOR_MODE_MANUAL_COMMUTATION, 0);
	}
}

void motor_set_wheel(uint8_t wheel_num, motor_mode_t mode, uint8_t pwm_level) {
	MOTOR_INDEX = wheel_num;
	if (mode == MOTOR_MODE_MANUAL_COMMUTATION) {
		MOTOR_CTL = motor_manual_commutation_patterns[wheel_num];
		MOTOR_PWM = pwm_level;
	} else if (mode == MOTOR_MODE_BRAKE) {
		MOTOR_CTL = 0b10101000;
		MOTOR_PWM = 0;
	} else {
		MOTOR_PWM = pwm_level;
		MOTOR_CTL = mode == MOTOR_MODE_BACKWARD ? 0b00000011 : 0b00000010;
	}
}

