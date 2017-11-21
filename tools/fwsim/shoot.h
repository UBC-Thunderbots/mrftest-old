#ifndef PRIMITIVES_SHOOT_H
#define PRIMITIVES_SHOOT_H

#include "primitive.h"


void shoot_tick();
void shoot_start(const primitive_params_t *params);


/**
 * component to build information along major axis, minor axis, or rotation
 */
typedef struct {
    float disp;
    float vel;
    float accel;
    float time;  
} Component;


/**
 * data container for components of the robot
 */
typedef struct {
    Component min;
    Component maj;
    Component rot;
} PhysBot;

#endif
