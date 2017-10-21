#include "shoot.h"
#include "../chicker.h"
#include "../control.h"
#include "../dr.h"
#include "../dribbler.h"
#include "../leds.h"
#include "../physics.h"
#include "../bangbang.h"
#include <math.h>
#include <stdio.h>

// these are set to decouple the 3 axis from each other
// the idea is to clamp the maximum velocity and acceleration
// so that the axes would never have to compete for resources
#define TIME_HORIZON 0.05f //s

static float destination[3], major_vec[2], minor_vec[2];

const float minor_disp_limit = 0;
const float rotation_limit = 0;

void set_displacement(float *dx, float *dy, dr_data_t states) {
	// relative distances to destination
	*dx = destination[0] - states.x; // along major axis
	*dy = destination[1] - states.y; // along minor axis
}

// gets distance along a vector
float vector_displacement(float vec[], float x, float y) {
	float vec2[2] = {x, y};
	return dot_product(vec, vec2); 
}

void plan_move(float *accel, float *time, float disp, float vel, float p[3]) {
	BBProfile profile;
	PrepareBBTrajectoryMaxV(&profile, disp, vel, p[0], p[1], p[2]); 
	PlanBBTrajectory(&profile);
	*accel = BBComputeAvgAccel(&profile, TIME_HORIZON);
	*time = GetBBTime(&profile);

}

void to_log(log_record_t *log, float timeTarget, float accel[3]) {
	log->tick.primitive_data[0] = destination[0];//accel[0];
	log->tick.primitive_data[1] = destination[1];//accel[1];
	log->tick.primitive_data[2] = destination[2];//accel[2];
	log->tick.primitive_data[3] = accel[0];//timeX;
	log->tick.primitive_data[4] = accel[1];//timeY;
	log->tick.primitive_data[5] = accel[2];
	log->tick.primitive_data[6] = timeTarget;
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
static void shoot_start(const primitive_params_t *params) {


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

	// arm the chicker
	chicker_auto_arm((params->extra & 1) ? CHICKER_CHIP : CHICKER_KICK, 
		params->params[3]);
	if (!(params->extra & 1)) {
		dribbler_set_speed(8000);
	}
	
}

/**
 * \brief Ends a movement of this type.
 *
 * This function runs when the host computer requests a new movement while a
 * shoot movement is already in progress.
 */
static void shoot_end(void) {
	chicker_auto_disarm();
}

/**
 * \brief Ticks a movement of this type.
 *
 * This function runs at the system tick rate while this primitive is active.
 *
 * \param[out] log the log record to fill with information about the tick, or
 * \c NULL if no record is to be filled
 */
static void shoot_tick(log_record_t *log) {
    //TODO: what would you like to log?
    // get the states
    dr_data_t current_states;
    dr_get(&current_states);
    // create velocity array
    float vel[3] = {current_states.vx, current_states.vy, current_states.avel};
    float angle = current_states.angle;
    // calculate global displacement
    float dx, dy;
    set_displacement(&dx, &dy, current_states);
    // get min angle between current angle relative to global x and 
    // destination angle relative to global x
    float to_rotate = min_angle_delta(angle, destination[2]);
    printf("Current Angle %f, Final Angle %f, To Rotate %f\n", angle, destination[2], to_rotate);
    // get displacement along major and minor axes
    float major_disp = vector_displacement(major_vec, dx, dy);
    float minor_disp = vector_displacement(minor_vec, dx, dy);
    //TODO: tune further: experimental
    float major_vel = vector_displacement(major_vec, vel[0], vel[1]);
    float minor_vel = vector_displacement(minor_vec, vel[0], vel[1]);
    float major_accel = 0, minor_accel = 0;
    float time_major = 0, time_minor = 0;
    if (abs(major_disp) > 0) {
        // haven't reached the ball yet
        // TODO: change hard coded numbers here
        float major_par[3] = { 1.0f, 1.0f, 1.0f };
        plan_move(&major_accel, &time_major, major_disp, major_vel, major_par);
    } 
    float minor_par[3] = {0, MAX_Y_A, MAX_Y_V};
    plan_move(&minor_accel, &time_minor, minor_disp, minor_vel, minor_par);
    float timeTarget = (time_minor > TIME_HORIZON) ? time_minor : TIME_HORIZON;
    // target rotation speed
    // float targetVel = 1.6f * to_rotate / timeTarget; 
    float targetVel = 1.6f * to_rotate / timeTarget; 
    // rotation acceleration
    float rot_accel = (targetVel - vel[2]) / TIME_HORIZON;
    // limit the rotation acceleration 
    Clamp(&rot_accel, MAX_T_A);
    float accel[3] = {0, 0, rot_accel};

    float local_x_norm_vec[2] = { cosf(angle), sinf(angle) }; 
    float local_y_norm_vec[2] = { -sinf(angle), cosf(angle) };

    float r = sqrtf(pow(major_disp, 2) + pow(major_disp, 2));
    float scale = 0.75;
    if (abs(minor_accel) < abs(major_accel) && r > 0.5) {
        if (dy > 0) {
            if (dx > 0) {
                major_accel = minor_accel * scale;
            } else {
                major_accel = -minor_accel * scale; 
            } 
        } else {
            if (dx > 0) {
                major_accel = -minor_accel * scale;
            } else {
                major_accel = minor_accel * scale; 
            } 
        }
    }

    accel[0] =  minor_accel * dot_product(local_x_norm_vec, minor_vec);
    accel[0] += major_accel * dot_product(local_x_norm_vec, major_vec); 
    accel[1] =  minor_accel * dot_product(local_y_norm_vec, minor_vec);
    accel[1] += major_accel * dot_product(local_y_norm_vec, major_vec); 

    apply_accel(accel, accel[2]); // accel is already in local coords
}



/**
 * \brief The shoot movement primitive.
 */
const primitive_t SHOOT_PRIMITIVE = {
	.direct = false,
	.init = &shoot_init,
	.start = &shoot_start,
	.end = &shoot_end,
	.tick = &shoot_tick,
};

