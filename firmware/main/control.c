#include "control.h"
#include "wheels.h"
#include <string.h>

//Maximum deviation from encoder 
//determinded setpoint in PWM levels
#define CURRENT_LIMIT 75.0

typedef struct {
	float integrator, saturation_difference, anti_windup_offset;
} PI_ctx_t;

static PI_ctx_t pis[4];

static void wheel_clear(PI_ctx_t *ctx) {
	ctx->integrator = 0;
	ctx->saturation_difference = 0;
	ctx->anti_windup_offset = 0;
}

void control_clear() {
	for (uint8_t i = 0; i < 4; ++i) {
		wheel_clear(&pis[i]);
	}
}

void control_process_new_setpoints(const int16_t setpoints[4]) {
	memcpy(wheels_setpoints, setpoints, sizeof(wheels_setpoints));
}

static int16_t wheel_iter(PI_ctx_t *ctx, int16_t feedback, int16_t setpoint) {
	int16_t error;
	float feedback_pwm_equiv, new_anti_windup_offset, error_compensated, control_action, plant_min, plant_max, plant;

	feedback_pwm_equiv = feedback * (1.0 / 3.0);
	error = setpoint - feedback;
	new_anti_windup_offset = 0.7992 * ctx->anti_windup_offset + 0.2852 * ctx->saturation_difference;
	error_compensated = error - new_anti_windup_offset;
	ctx->integrator += error_compensated;
	control_action = error_compensated * 1.1072 + ctx->integrator * 0.2869;
	plant = control_action;
	plant_min = feedback_pwm_equiv - CURRENT_LIMIT;
	plant_max = feedback_pwm_equiv + CURRENT_LIMIT;
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

void control_tick(void) {
	for (uint8_t i = 0; i < 4; ++i) {
		wheels_drives[i] = wheel_iter(&pis[i], wheels_encoder_counts[i], wheels_setpoints[i]);
	}
}


