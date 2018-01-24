#ifndef PRIMITIVES_MOVE_H
#define PRIMITIVES_MOVE_H

#include "primitive.h"
#include "physics.h"

extern const primitive_t MOVE_PRIMITIVE;

static const float CLOSEST_WHEEL_ANGLE = 30.0f * M_PI / 180.0f;

#define VAL_EQUIVALENT_2_ZERO (5e-3f)
#define CONTROL_TICK (1.0f/CONTROL_LOOP_HZ)

#define LOOK_AHEAD_T 10

#endif
