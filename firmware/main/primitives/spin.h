#ifndef PRIMITIVES_SPIN_H
#define PRIMITIVES_SPIN_H

#include "primitive.h"
#include "../physics.h"
extern const primitive_t SPIN_PRIMITIVE;

#ifndef FWSIM

#else
void spin_start(primitive_params_t *p);
void spin_tick();
#endif

#endif
