#include "wheels.h"
#include "control.h"
#include "encoder.h"
#include "io.h"
#include "led.h"
#include "mac.h"
#include "motor.h"
#include "mrf.h"

wheel_mode_t wheel_mode = WHEEL_MODE_COAST;
int16_t wheel_setpoint[4] = { 0, 0, 0, 0 };

static control_ctx_t control_contexts[4];

void wheels_tick() {
	switch (wheel_mode) {
		case WHEEL_MODE_COAST:
			for (uint8_t i = 0; i < 4; ++i) {
				set_wheel(i, FLOAT, 0);
				control_clear(&control_contexts[i]);
				read_encoder(i);
			}
			break;

		case WHEEL_MODE_BRAKE:
			for (uint8_t i = 0; i < 4; ++i) {
				set_wheel(i, BRAKE, 0);
				control_clear(&control_contexts[i]);
				read_encoder(i);
			}
			break;

		case WHEEL_MODE_OPEN_LOOP:
			for (uint8_t i = 0; i < 4; ++i) {
				int16_t output = wheel_setpoint[i];
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
				control_clear(&control_contexts[i]);
				read_encoder(i);
			}
			break;

		case WHEEL_MODE_CLOSED_LOOP:
			for (uint8_t i = 0; i < 4; ++i) {
				int16_t enc_val = read_encoder(i);
				int16_t output = control_iter(wheel_setpoint[i], enc_val, &control_contexts[i]);
				if(output > 0) {
					set_wheel(i, FORWARD, output);
				} else if(output < 0) {
					set_wheel(i, BACKWARD, -output);
				} else {
					set_wheel(i, BRAKE, 0);
				}
			}
			break;
	}
}

