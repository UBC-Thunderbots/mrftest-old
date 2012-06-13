#include "control.h"

float forward[CONTROL_ORDER] = { 4.5987234, 2.92375e-4 };
float feed_back[CONTROL_ORDER - 1] = { 5.9385e-3 };

control_state_t controller_state[4];

int16_t control_iteration(control_state_t control_state, int16_t wheel_setpoint, int16_t encoder) {
	float diff = wheel_setpoint - encoder;
	int16_t output = 0;
	for(uint8_t i = CONTROL_ORDER; i>0;i--) {
		diff += -1 * feed_back[i - 1] * control_state[i];
		control_state[i] = control_state[i - 1];
		output += control_state[i] * forward[i];
	}

	if(output > 255)
		output = 255;
	if(output < -255)
		output = -255;

	return output;
}

