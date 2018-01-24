#ifndef QUADRATIC_H
#define QUADRATIC_H

#include "physbot.h"

void quad_optimize(float forces[4], PhysBot pb, dr_data_t state, float a_req[3]);

#endif