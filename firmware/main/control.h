#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

#define CONTROL_ORDER 2

typedef float control_state_t[CONTROL_ORDER];

extern control_state_t controller_state[4];
extern float forward[CONTROL_ORDER];
extern float feed_back[CONTROL_ORDER-1];

int16_t control_iteration(control_state_t control_state, int16_t wheel_setpoint, int16_t encoder);

#endif

