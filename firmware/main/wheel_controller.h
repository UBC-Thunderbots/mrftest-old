#ifndef WHEEL_CONTROLLER_H
#define WHEEL_CONTROLLER_H

#include <stdint.h>

typedef struct {
	float integrator;
	float saturation_difference;
	float anti_windup_offset;
} wheel_controller_ctx_t;

void wheel_controller_clear(wheel_controller_ctx_t *ctx);

int16_t wheel_controller_iter(int16_t setpoint, int16_t feedback, wheel_controller_ctx_t *ctx);

#endif

