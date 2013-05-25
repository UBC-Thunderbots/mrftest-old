#include "control.h"
#include "buffers.h"

//Maximum deviation from encoder 
//determinded setpoint in PWM levels
#define CURRENT_LIMIT 75.0

static control_ctx_t control_context;

void control_setpoint_changed(int16_t setpoints[4]) {
	for(uint8_t i=0;i<4;++i) {
		control_context.setpoints[i]=setpoints[i];
	}
}

void wheel_clear(PI_ctx_t *ctx) {
	ctx->integrator = 0;
	ctx->saturation_difference = 0;
	ctx->anti_windup_offset = 0;
}

void control_clear() {
	for(uint8_t i=0;i<4;i++) {
		wheel_clear(&(control_context.wheels[i]));
	}
}

int16_t wheel_iter(PI_ctx_t *ctx, int16_t feedback, int16_t setpoint) {
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

void control_iter(int16_t feedback[4],int16_t outputs[4]) {
	for(int i=0;i<4;i++) {
		current_buffer->tick.setpoint[i] = control_context.setpoints[i];
		outputs[i] = wheel_iter(&(control_context.wheels[i]),feedback[i],control_context.setpoints[i]);
	}
}


