#include "wheels.h"
#include "control.h"
#include "encoder.h"
#include "io.h"
#include "led.h"
#include "mac.h"
#include "motor.h"
#include "mrf.h"
#include "sdcard.h"

wheels_mode_t wheels_mode = WHEELS_MODE_MANUAL_COMMUTATION;
wheels_setpoints_t wheels_setpoints;
int16_t wheels_encoder_counts[4] = { 0, 0, 0, 0 };
int16_t wheels_drives[4] = { 0, 0, 0, 0 };

void wheels_tick(float battery) {
	// Read optical encoders.
	for (uint8_t i = 0; i < 4; ++i) {
		wheels_encoder_counts[i] = read_encoder(i);
	}

	// Run the controller if necessary and drive the motors.
	switch (wheels_mode) {
		case WHEELS_MODE_MANUAL_COMMUTATION:
			control_clear();
			for (uint8_t i = 0; i < 4; ++i) {
				motor_set_wheel(i, MOTOR_MODE_MANUAL_COMMUTATION, wheels_setpoints.wheels[i]);
			}
			break;

		case WHEELS_MODE_BRAKE:
			control_clear();
			for (uint8_t i = 0; i < 4; ++i) {
				motor_set_wheel(i, MOTOR_MODE_BRAKE, 0);
			}
			break;

		case WHEELS_MODE_OPEN_LOOP:
			control_clear();
			for (uint8_t i = 0; i < 4; ++i) {
				int16_t output = wheels_setpoints.wheels[i];
				if (output < -255) {
					output = -255;
				} else if (output > 255) {
					output = 255;
				}
				wheels_drives[i] = output;
				if (output >= 0) {
					motor_set_wheel(i, MOTOR_MODE_FORWARD, output);
				} else if (output < 0) {
					motor_set_wheel(i, MOTOR_MODE_BACKWARD, -output);
				}
			}
			break;

		case WHEELS_MODE_CLOSED_LOOP:
			control_tick(battery);
			uint8_t encoders_fail = ENCODER_FAIL;
			for (uint8_t i = 0; i < 4; ++i) {
				if (encoders_fail & 1) {
					motor_set_wheel(i, MOTOR_MODE_MANUAL_COMMUTATION, 0);
				} else if (wheels_drives[i] >= 0) {
					motor_set_wheel(i, MOTOR_MODE_FORWARD, wheels_drives[i]);
				} else {
					motor_set_wheel(i, MOTOR_MODE_BACKWARD, -wheels_drives[i]);
				}
				encoders_fail >>= 1;
			}
			break;
	}
}

