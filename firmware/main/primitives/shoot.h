#ifndef PRIMITIVES_SHOOT_H
#define PRIMITIVES_SHOOT_H

#include "primitive.h"

#ifndef FWSIM
extern const primitive_t SHOOT_PRIMITIVE;
#else
void shoot_tick();
void shoot_start(const primitive_params_t *params);
#endif

#endif

