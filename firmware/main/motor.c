#include "motor.h"
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
		outb(MOTOR_INDEX, wheel_num);
		if (direction == MANUAL_COMMUTATION) {
			outb(MOTOR_CTL, motor_manual_commutation_patterns[wheel_num]);
			outb(MOTOR_PWM, pwm_level);
		} else if (direction == BRAKE) {
			outb(MOTOR_CTL, 0b10101000);
			outb(MOTOR_PWM, 0);
		} else {
			outb(MOTOR_PWM, pwm_level);
			outb(MOTOR_CTL, direction == BACKWARD ? 0b00000011 : 0b00000010);
		}
	}
}

void set_dribbler(direction_t direction, uint8_t pwm_level) {
	set_wheel(4, direction, pwm_level);
}

