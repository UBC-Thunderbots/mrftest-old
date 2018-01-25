#ifndef PRIMITIVES_SPIN_H
#define PRIMITIVES_SPIN_H

#include "primitive.h"
#include "../physics.h"

#ifndef FWSIM
extern const primitive_t SPIN_PRIMITIVE;
#else
void spin_start(primitive_params_t *p);
void spin_tick();
#endif

#endif
