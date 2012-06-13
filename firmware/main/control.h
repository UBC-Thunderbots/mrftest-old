#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

typedef struct {
	float integrator, saturation_difference, anti_windup_offset;
} control_ctx_t;

void control_clear(control_ctx_t *ctx);
int16_t control_iter(int16_t setpoint, int16_t feedback, control_ctx_t *ctx);

#endif

