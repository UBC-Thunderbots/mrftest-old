#include "shoot.h"
#include "../chicker.h"
#include "../control.h"
#include "../dr.h"
#include "../dribbler.h"
#include "../leds.h"
#include "../physics.h"
#include "../bangbang.h"
#include "../util/log.h"
#include "../util/physbot.h"
#include <math.h>
#include <stdio.h>

// these are set to decouple the 3 axis from each other
// the idea is to clamp the maximum velocity and acceleration
// so that the axes would never have to compete for resources
#define TIME_HORIZON 0.05f //s

//TODO: find out actual wheel angles
const float CLOSEST_WHEEL_ANGLE = 30.0f * M_PI / 180.0f;
const float PI_2 = M_PI / 2.0f;
const float APPROACH_LIMIT = 2 * M_PI * ROBOT_RADIUS;
static float destination[3], end_speed, major_vec[2], minor_vec[2];
// Only need two data points to form major axis vector.

void choose_rotation_destination(PhysBot *pb, float angle) {
	// if we are close enough then we should just allow the bot to rotate
	// onto its destination angle
	if ((float) fabs(pb->maj.disp) > APPROACH_LIMIT) {
		float left_perpindicular = angle + CLOSEST_WHEEL_ANGLE - PI_2;
		float right_perpindicular = angle - CLOSEST_WHEEL_ANGLE + PI_2;
		float theta_norm = atan2f(pb->dr[1], pb->dr[0]);
		float to_left = min_angle_delta(left_perpindicular, theta_norm);
		float to_right = min_angle_delta(right_perpindicular, theta_norm);
		// pick the closer axis to rotate onto
		pb->rot.disp = (fabs(to_right) < fabs(to_left)) ? to_right : to_left;
	}
}

void move_to_log(log_record_t *log, float time_target, float accel[3]) {
    log_destination(log, destination);
    log_accel(log, accel);
    log_time_target(log, time_target);
}

void plan_move_rotation(PhysBot *pb, dr_data_t state) {
	pb->rot.time = (pb->maj.time > TIME_HORIZON) ? pb->maj.time : TIME_HORIZON;
	pb->rot.vel = pb->rot.disp / pb->rot.time * 4.0f; 
	pb->rot.accel = (pb->rot.vel - state.avel) / TIME_HORIZON;
}

/**
 * \brief Initializes the move primitive.
 *
 * This function runs once at system startup.
 */
static void move_init(void) 
{
	// Currently has nothing to do
}

/**
 * \brief Starts a movement of this type.
 *
 * This function runs each time the host computer requests to start a move
 * movement.
 *
 * \param[in] params the movement parameters, which are only valid until this
 * function returns and must be copied into this module if needed
 */
static void move_start(const primitive_params_t *params) 
{
	//Parameters: 	destination_x [mm]
	//				destination_y [mm]
	//				destination_ang [centi-rad]
	//				end_speed [millimeter/s]
  
	// Convert into m/s and rad/s because physics is in m and s
	destination[0] = (float) (params->params[0]) / 1000.0f;
	destination[1] = (float) (params->params[1]) / 1000.0f;
	destination[2] = (float) (params->params[2]) / 100.0f;
	end_speed = (float) (params->params[3]) / 1000.0f;
	
	dr_data_t current_states;
	dr_get(&current_states);
	
	float dx = destination[0] - current_states.x;
	float dy = destination[1] - current_states.y;
	float total_disp = norm2(dx, dy);	
	
	major_vec[0] = dx / total_disp; 
	major_vec[1] = dy / total_disp;
	minor_vec[0] = major_vec[0];
	minor_vec[1] = major_vec[1];
	rotate(minor_vec, M_PI/2);	
}

/**
 * \brief Ends a movement of this type.
 *
 * This function runs when the host computer requests a new movement while a
 * move movement is already in progress.
 */
static void move_end(void) 
{
}

/**
 * \brief Ticks a movement of this type.
 *
 * This function runs at the system tick rate while this primitive is active.
 *
 * \param[out] log the log record to fill with information about the tick, or
 * \c NULL if no record is to be filled
 */
static void move_tick(log_record_t *log) {
	dr_data_t current_states;
	dr_get(&current_states);
	PhysBot pb = setup_bot(current_states, destination, major_vec, minor_vec);

	choose_rotation_destination(&pb, current_states.angle);

	if (pb.maj.disp > 0) {
		// magic constants
		float major_par[3] = {end_speed, 2.5, 2.5};
		plan_move(&pb.maj, major_par);
	}
	// magic constants
	float minor_par[3] = {0, 1.5, 1.5};
	plan_move(&pb.min, minor_par);

	plan_move_rotation(&pb, current_states);

	float accel[3] = {0, 0, pb.rot.accel};
	to_local_coords(accel, pb, current_states.angle, major_vec, minor_vec);
	apply_accel(accel, accel[2]);

    if (log) { move_to_log(log, pb.rot.time, accel); }
}

/**
 * \brief The move movement primitive.
 */
const primitive_t MOVE_PRIMITIVE = {
	.direct = false,
 	.init = &move_init,
	.start = &move_start,
	.end = &move_end,
	.tick = &move_tick,
};

