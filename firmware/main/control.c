#include "control.h"

void control_clear(control_ctx_t *ctx) {
	ctx->integrator = 0;
	ctx->saturation_difference = 0;
	ctx->anti_windup_offset = 0;
}

int16_t control_iter(int16_t setpoint, int16_t feedback, control_ctx_t *ctx) {
	int16_t error;
	float feedback_pwm_equiv, new_anti_windup_offset, error_compensated, control_action, plant_min, plant_max, plant;

	feedback_pwm_equiv = feedback / 3.0;
	error = setpoint - feedback;
	new_anti_windup_offset = 0.7992 * ctx->anti_windup_offset + 0.2852 * ctx->saturation_difference;
	error_compensated = error - new_anti_windup_offset;
	ctx->integrator += error_compensated;
	control_action = error_compensated * 1.1072 + ctx->integrator * 0.2869;
	plant = control_action;
	plant_min = feedback_pwm_equiv - 45.0;
	plant_max = feedback_pwm_equiv + 45.0;
	if (plant < plant_min) {
		plant = plant_min;
	} else if (plant > plant_max) {
		plant = plant_max;
	}
	if (plant < -255) {
		plant = -255;
	} else if (plant > 255) {
		plant = 255;
	}
	ctx->saturation_difference = control_action - plant;
	ctx->anti_windup_offset = new_anti_windup_offset;
	return (int16_t) plant;
}

