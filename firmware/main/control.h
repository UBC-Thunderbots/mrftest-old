#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

typedef struct {
	float integrator, saturation_difference, anti_windup_offset;
} PI_ctx_t;

typedef struct {
	PI_ctx_t wheels[4];
	int16_t setpoints[4];
} control_ctx_t;

void control_clear();
void control_setpoint_changed(int16_t setpoints[4]);
void control_iter(int16_t feedback[4], int16_t outputs[4]);

#endif

