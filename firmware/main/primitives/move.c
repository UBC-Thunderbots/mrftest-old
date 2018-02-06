#include "move.h"
#include "../chicker.h"
#include "../control.h"
#include "../dr.h"
#include "../dribbler.h"
#include "../leds.h"
#include "../bangbang.h"
#include "../util/log.h"
#include "../physics.h"
#include "../util/physbot.h"
#include <math.h>
#include <stdio.h>

// these are set to decouple the 3 axis from each other
// the idea is to clamp the maximum velocity and acceleration
// so that the axes would never have to compete for resources
#define TIME_HORIZON 0.05f //s

const float PI_2 = M_PI / 2.0f;
static float destination[3], end_speed, major_vec[2], minor_vec[2];

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

/**
 * Pass information to be logged.
 * 
 * @param log The log object.
 * @param time_target The time target to log
 * @param accel A 3 length array of {x, y, rotation} accelerations
 */ 
void move_to_log(log_record_t *log, float time_target, float accel[3]) {
    log_destination(log, destination);
    log_accel(log, accel);
    log_time_target(log, time_target);
}


void plan_move_rotation(PhysBot *pb, float avel) {
	pb->rot.time = (pb->maj.time > TIME_HORIZON) ? pb->maj.time : TIME_HORIZON;
	pb->rot.vel = pb->rot.disp / pb->rot.time * 4.0f; 
	pb->rot.accel = (pb->rot.vel - avel) / TIME_HORIZON;
	pb->rot.accel = (fabs(pb->rot.accel) < MAX_T_A) ? pb->rot.accel : MAX_T_A; 
}


// need the ifndef here so that we can ignore this code when compiling
// the firmware tests
#ifndef FWTEST
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
	float total_disp = sqrtf(dx * dx + dy * dy);	
	
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

	// magic constants
	float major_par[3] = {end_speed, 2.5f, 2.5f};
	plan_move(&pb.maj, major_par);
	// magic constants
	float minor_par[3] = {0.0f, 1.5f, 1.5f};
	plan_move(&pb.min, minor_par);

	plan_move_rotation(&pb, current_states.avel);

	float accel[3] = {0.0f, 0.0f, pb.rot.accel};
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
#endif

