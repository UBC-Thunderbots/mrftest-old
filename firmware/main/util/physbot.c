#include "physbot.h"
#include "../dr.h"
#include "../bangbang.h"
#include "../physics.h"

// sets up the PhysBot container with all of its info
PhysBot setup_bot(dr_data_t states, float destination[3], float major_vec[2], 
    float minor_vec[2]) {
    float v[2] = {states.vx, states.vy};
    float dr[2] = {destination[0] - states.x, destination[1] - states.y};
    PhysBot pb = {
        .dr = {*dr},
        .rot = {
            .disp = min_angle_delta(states.angle, destination[2])
        },
        .maj = {
            .disp = dot2D(major_vec, dr),
            .vel = dot2D(major_vec, v),
            .accel = 0,
            .time = 0
        },
        .min = {
            .disp = dot2D(minor_vec, dr),
            .vel = dot2D(minor_vec, v),
            .accel = 0,
            .time = 0
        },
        .major_vec = {*major_vec},
        .minor_vec = {*minor_vec}
    };
    return pb;
}

#ifndef FWTEST
/**
 * Creates the BBProfile for a component. It is assumed that the displacement, 
 * velocity, and acceleration lie along the major or minor axis (i.e. the 
 * Component given is a major or minor axis component). 
 */
void plan_move(Component *c, float p[3]) {
    BBProfile profile;
    PrepareBBTrajectoryMaxV(&profile, c->disp, c->vel, p[0], p[1], p[2]); 
    PlanBBTrajectory(&profile);
    c->accel = BBComputeAvgAccel(&profile, TIME_HORIZON);
    c->time = GetBBTime(&profile);
}
#endif

/**
 * Uses a rotaion matrix to rotate the acceleration vectors of the given 
 * PhysBot back to local xy coordinates and store them in a separate array. The
 * given angle should be the bot's angle relative to the global x-axis.
 */
void to_local_coords(float accel[3], PhysBot pb, float angle, float major_vec[2], 
    float minor_vec[2]) {
    float local_norm_vec[2][2] = {
        {cosf(angle), sinf(angle)}, 
        {cosf(angle + M_PI / 2), sinf(angle + M_PI / 2)}
    };
    for (int i = 0; i < 2; i++) {
        accel[i] =  pb.min.accel * dot2D(local_norm_vec[i], minor_vec);
        accel[i] += pb.maj.accel * dot2D(local_norm_vec[i], major_vec); 
    }
}