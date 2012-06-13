#include "motor.h"
#include "io.h"

void motor_scram() {
	wheel_scram();
	motor_scram();
}

uint8_t read_wheel_pwm(uint8_t num) {
	switch(num) {
		case 0: return inb(WHEEL0_PWM);
		case 1: return inb(WHEEL1_PWM);
		case 2: return inb(WHEEL2_PWM);
		case 3: return inb(WHEEL3_PWM);
		default: return 0xFF;
	}
}

void write_wheel_pwm(uint8_t num, uint8_t value) {
	switch(num) {
		case 0:
			outb(WHEEL0_PWM,value);
			break;
		case 1:
			outb(WHEEL1_PWM,value);
			break;
		case 2:
			outb(WHEEL2_PWM,value);
			break;
		case 3:
			outb(WHEEL3_PWM,value);
			break;
	}
}

void wheel_scram() {
	outb(WHEEL_CTL,0x00);
	for (uint8_t index = 0; index < 4; index++) {
		write_wheel_pwm(index, 0x00);
	}
}

void set_wheel(uint8_t wheel_num, direction_t direction, uint8_t pwm_level) {
	if (wheel_num <= 3) {
		uint8_t readback = inb(WHEEL_CTL);
		readback = (readback & ~(0x03 << (wheel_num * 2))) | (direction << (wheel_num * 2));
		write_wheel_pwm(wheel_num, pwm_level);
		outb(WHEEL_CTL, readback);
	}
}

void dribbler_scram() {
	outb(DRIBBLER_CTL, 0x00);
	outb(DRIBBLER_PWM, 0x00);
}

void set_dribbler(direction_t direction, uint8_t pwm_level) {
	outb(DRIBBLER_CTL, direction);
	outb(DRIBBLER_PWM, pwm_level);
}

