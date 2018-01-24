#ifndef PHYSBOT_H
#define PHYSBOT_H

#include "../dr.h"

#define TIME_HORIZON 0.05f //s

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
    float dr[2];
    float major_vec[2];
    float minor_vec[2];
    Component min;
    Component maj;
    Component rot;
} PhysBot;

PhysBot setup_bot(dr_data_t states, float destination[3], float major_vec[2], 
    float minor_vec[2]);

void plan_move(Component *c, float p[3]);

void to_local_coords(float accel[3], PhysBot pb, float angle, float major_vec[2], 
    float minor_vec[2]);

#endif
