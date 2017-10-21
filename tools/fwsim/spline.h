#ifndef PRIMITIVES_MOVE_H
#define PRIMITIVES_MOVE_H

#include "physics.h"
#include "primitive.h"


#define VAL_EQUIVALENT_2_ZERO (5e-3f)
#define CONTROL_TICK (1.0f/CONTROL_LOOP_HZ)

#define LOOK_AHEAD_T 10

void spline_tick();
void spline_start(primitive_params_t *params);
#endif
