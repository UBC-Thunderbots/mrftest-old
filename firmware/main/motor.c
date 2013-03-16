#include "motor.h"
#include "io.h"

void motor_scram() {
	for (uint8_t index = 0; index <= 4; ++index) {
		set_wheel(index, FLOAT, 0);
	}
}

void set_wheel(uint8_t wheel_num, direction_t direction, uint8_t pwm_level) {
	if (wheel_num <= 4) {
		outb(MOTOR_INDEX, wheel_num);
		if (direction == FLOAT) {
			outb(MOTOR_CTL, 0);
		} else if (direction == BRAKE) {
			outb(MOTOR_CTL, 0b10101000);
		} else {
			outb(MOTOR_CTL, 0);
			outb(MOTOR_PWM, pwm_level);
			outb(MOTOR_CTL, direction == BACKWARD ? 0b00000011 : 0b00000010);
		}
	}
}

void set_dribbler(direction_t direction, uint8_t pwm_level) {
	set_wheel(4, direction, pwm_level);
}

