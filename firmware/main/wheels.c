#include "wheels.h"
#include "control.h"
#include "encoder.h"
#include "io.h"
#include "led.h"
#include "mac.h"
#include "motor.h"
#include "mrf.h"

wheel_ctx_t wheel_context;

void wheel_update_ctx() {
	control_setpoint_changed(wheel_context.setpoints);
}

void wheels_tick() {
	int16_t enc_val[4];
	int16_t outputs[4];
	switch (wheel_context.mode) {
		case WHEEL_MODE_MANUAL_COMMUTATION:
			control_clear();
			for (uint8_t i = 0; i < 4; ++i) {
				set_wheel(i, MANUAL_COMMUTATION, wheel_context.setpoints[i]);
				read_encoder(i);
			}
			break;

		case WHEEL_MODE_BRAKE:
			control_clear();
			for (uint8_t i = 0; i < 4; ++i) {
				set_wheel(i, BRAKE, 0);
				read_encoder(i);
			}
			break;

		case WHEEL_MODE_OPEN_LOOP:
			control_clear();
			for (uint8_t i = 0; i < 4; ++i) {
				int16_t output = wheel_context.setpoints[i];
				if (output < -255) {
					output = -255;
				} else if (output > 255) {
					output = 255;
				}
				if (output > 0) {
					set_wheel(i, FORWARD, output);
				} else if (output < 0) {
					set_wheel(i, BACKWARD, -output);
				} else {
					set_wheel(i, BRAKE, 0);
				}
				read_encoder(i);
			}
			break;

		case WHEEL_MODE_CLOSED_LOOP:
			for (uint8_t i = 0; i < 4; ++i) {
				enc_val[i] = read_encoder(i);
			}
			control_iter(enc_val, outputs);
			for (uint8_t i = 0; i < 4; ++i) {
				if (outputs[i] >= 0) {
					set_wheel(i, FORWARD, outputs[i]);
				} else {
					set_wheel(i, BACKWARD, -outputs[i]);
				}
			}
			break;
	}
}

