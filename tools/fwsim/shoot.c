#include "shoot.h"
#include "control.h"
#include "simulate.h"
#include "physics.h"
#include "bangbang.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// these are set to decouple the 3 axis from each other
// the idea is to clamp the maximum velocity and acceleration
// so that the axes would never have to compete for resources
#define TIME_HORIZON 0.05f //s
#define NUM_SPLINE_POINTS 50

static float destination[3], major_vec[2], minor_vec[2], total_rot;

// sets up the PhysBot container with all of its info
PhysBot setup_bot(dr_data_t states) {
    float v[2] = {states.vx, states.vy};
    float dr[2] = {destination[0] - states.x, destination[1] - states.y};
    PhysBot pb = {
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
        }
    };
    return pb;
}

/**
 * Scales the major acceleration by the distance from the major axis and the
 * amount required left to rotate. Total roation and the distance vector should
 * not be zero so as to avoid divide by zero errors.
 */
void scale(PhysBot *pb) {
    float x = (float) fabs(pb->maj.disp) - ROBOT_RADIUS;
    float r = sqrtf(pow(x, 2) + pow(pb->min.disp, 2));
    if (r != 0) {
        float abs_factor = ((float) fabs(pb->min.disp)) / r;
        float minor_axis_factor = 1 - abs_factor;
        pb->maj.accel *= minor_axis_factor;
    }

    if (total_rot != 0) {
        float rot_factor = 1 - ((float) fabs(pb->rot.disp / total_rot));
        pb->maj.accel *= rot_factor;
    }
}

/**
 * Determines the rotation acceleration after setup_bot has been used and
 * plan_move has been done along the minor axis. The minor time from bangbang
 * is used to determine the rotation time, and thus the rotation velocity and
 * acceleration. The rotational acceleration is clamped under the MAX_T_A.
 */ 
void plan_rotation(PhysBot *pb, dr_data_t states) {
    pb->rot.time = (pb->min.time > TIME_HORIZON) ? pb->min.time : TIME_HORIZON;
    pb->rot.vel = 1.6f * pb->rot.disp / pb->rot.time; 
    pb->rot.accel = (pb->rot.vel - states.avel) / TIME_HORIZON;
    Clamp(&pb->rot.accel, MAX_T_A);
}

/**
 * Uses a rotaion matrix to rotate the acceleration vectors of the given 
 * PhysBot back to local xy coordinates and store them in a separate array. The
 * given angle should be the bot's angle relative to the global x-axis.
 */
void to_local_coords(float accel[3], PhysBot pb, float angle) {
    float local_norm_vec[2][2] = {
        {cosf(angle), sinf(angle)}, 
        {cosf(angle + M_PI / 2), sinf(angle + M_PI / 2)}
    };
    for (int i = 0; i < 2; i++) {
        accel[i] =  pb.min.accel * dot2D(local_norm_vec[i], minor_vec);
        accel[i] += pb.maj.accel * dot2D(local_norm_vec[i], major_vec); 
    }
}

// Creates the BBProfile for a component
void plan_move(Component *c, float p[3])
{
    BBProfile profile;
    PrepareBBTrajectoryMaxV(&profile, c->disp, c->vel, p[0], p[1], p[2]);
    PlanBBTrajectory(&profile);
    c->accel = BBComputeAvgAccel(&profile, TIME_HORIZON);
    c->time  = GetBBTime(&profile);
}

// Scales the major acceleration by the distance from the major axis
void scale(PhysBot *pb)
{
    float r          = sqrtf(pow(pb->maj.disp, 2) + pow(pb->min.disp, 2));
    float abs_factor = abs(pb->min.disp * 1000) / (r * 1000.0f);
    pb->maj.accel *= (1 - abs_factor);
}

// Only need two data points to form major axis vector.

/**
 * \brief Initializes the shoot primitive.
 *
 * This function runs once at system startup.
 */
static void shoot_init(void) {
}

/**
 * \brief Starts a movement of this type.
 *
 * This function runs each time the host computer requests to start a shoot
 * movement.
 *
 * \param[in] params the movement parameters, which are only valid until this
 * function returns and must be copied into this module if needed
 *	there are two shoot methods the second bit in extra byte indicate which
 *	method is called
 *
 *       params[0] = dest.x * 1000.0;
 *       params[1] = dest.y * 1000.0;
 *       params[2] = 0.0;
 *       params[3] = power * 1000.0;
 *       extra = chip;
 *
 *	method two
 *
 *       params[0] = dest.x * 1000.0;
 *       params[1] = dest.y * 1000.0;
 *       params[2] = orientation.angle_mod().to_radians() * 100.0;
 *       params[3] = power * 1000.0;
 *       extra = static_cast<uint8_t>(2 | chip);
 *
 *	What this function do
 *	1. record the movement intent
 *	2. there is no need to worry about recording the start position
 *	   because the primitive start function already does it
 *
 */
void shoot_start(const primitive_params_t *params) {


    // Convert into m/s and rad/s because physics is in m and s
    destination[0] = ((float) (params->params[0]) / 1000.0f);
    destination[1] = ((float) (params->params[1]) / 1000.0f);
    destination[2] = ((float) (params->params[2]) / 100.0f);
    

    // cosine and sine of orientation angle to global x axis
    major_vec[0] = cosf(destination[2]); 
    major_vec[1] = sinf(destination[2]);
    minor_vec[0] = major_vec[0];
    minor_vec[1] = major_vec[1];
    rotate(minor_vec, M_PI / 2);    

    dr_data_t states;
    dr_get(&states);
    total_rot = min_angle_delta(destination[2], states.angle);  
    // arm the chicker
    // chicker_auto_arm((params->extra & 1) ? CHICKER_CHIP : CHICKER_KICK, 
    //     params->params[3]);
    // if (!(params->extra & 1)) {
    //     dribbler_set_speed(8000);
    // }
    
}

/**
 * \brief Ends a movement of this type.
 *
 * This function runs when the host computer requests a new movement while a
 * shoot movement is already in progress.
 */
static void shoot_end(void) {
    // chicker_auto_disarm();
}

/**
 * \brief Ticks a movement of this type.
 *
 * This function runs at the system tick rate while this primitive is active.
 *
 * \param[out] log the log record to fill with information about the tick, or
 * \c NULL if no record is to be filled
 */
void shoot_tick() {
    dr_data_t states;
    dr_get(&states);
    PhysBot pb = setup_bot(states);
    //TODO: tune further: experimental
    if (pb.maj.disp > 0) {
        // haven't reached the ball yet
        // TODO: change hard coded numbers here
        float major_par[3] = { 1.0f, MAX_X_A * 0.75f, MAX_X_V };
        plan_move(&pb.maj, major_par);
    } 
    float minor_par[3] = {0, MAX_Y_A, MAX_Y_V * 2};
    plan_move(&pb.min, minor_par);
    plan_rotation(&pb, states);
    float accel[3] = {0, 0, pb.rot.accel};
    scale(&pb);
    to_local_coords(accel, pb, states.angle);
    apply_accel(accel, accel[2]);

    // if (log) {to_log(log, pb.rot.time, accel);}
}

