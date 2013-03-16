#include "motor.h"
#include "io.h"

void motor_scram() {
	wheel_scram();
	dribbler_scram();
}

uint8_t read_wheel_pwm(uint8_t num) {
	outb(MOTOR_INDEX, num);
	return inb(MOTOR_PWM);
}

void write_wheel_pwm(uint8_t num, uint8_t value) {
	outb(MOTOR_INDEX, num);
	outb(MOTOR_PWM, value);
}

void wheel_scram() {
	for (uint8_t index = 0; index < 4; ++index) {
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

void dribbler_scram() {
	set_wheel(4, FLOAT, 0);
}

void set_dribbler(direction_t direction, uint8_t pwm_level) {
	set_wheel(4, direction, pwm_level);
}

