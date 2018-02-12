#include "move.h"
#include "../chicker.h"
#include "../control.h"
#include "../dr.h"
#include "../dribbler.h"
#include "../leds.h"
#include "../physics.h"
#include "../bangbang.h"
#include "../util/physbot.h"
#include "../util/robot_constants.h"
#include "../util/log.h"
#include "../util/util.h"
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
	// onto its destination angle, so skip this if block
	if ((float) fabs(pb->maj.disp) > APPROACH_LIMIT) {
		// The axes perpindiculer to each of the wheels of the bot
		float perpindicular_axes_to_wheels[4] = {
			angle + ANGLE_TO_FRONT_WHEELS - PI_2,
			angle - ANGLE_TO_FRONT_WHEELS + PI_2,
			angle + ANGLE_TO_BACK_WHEELS - PI_2,
			angle - ANGLE_TO_BACK_WHEELS + PI_2
		};
		// the angle on the global axis corresponding to the bot's movement
		float theta_norm = atan2f(pb->dr[1], pb->dr[0]);
		// the angle between the bot's movement and each of the perpindicular 
		// wheel axes
		float relative_angles[NUMBER_OF_WHEELS];
		// the absolute value of those angles
		float absolute_value_relative_angles[NUMBER_OF_WHEELS];
		unsigned i;
		for (i = 0; i < NUMBER_OF_WHEELS; i++) {
			relative_angles[i] = min_angle_delta(perpindicular_axes_to_wheels[i], theta_norm);
		}
		// make an absolute value array of the angles to find the min
		fabs_of_array(relative_angles, absolute_value_relative_angles, NUMBER_OF_WHEELS);
		// get the index of the closest axis
		unsigned min_index = argmin(absolute_value_relative_angles, NUMBER_OF_WHEELS);
		// set the displacement
		pb->rot.disp = relative_angles[min_index];
	}
}


void plan_move_rotation(PhysBot *pb, float avel) {
	float time_target = (pb->maj.time > TIME_HORIZON) ? pb->maj.time : TIME_HORIZON;
	if (time_target > 0.5f){
		time_target = 0.5f;
	}	
	pb->rot.time = time_target;
	pb->rot.vel = pb->rot.disp / pb->rot.time; 
	pb->rot.accel = (pb->rot.vel - avel) / TIME_HORIZON;
	limit(&pb->rot.accel, MAX_T_A);
}

/**
 * Pass information to be logged.
 * 
 * @param log The log object.
 * @param time_target The time target to log
 * @param accel A 3 length array of {x, y, rotation} accelerations
 * @return void
 */ 
void move_to_log(log_record_t *log, float time_target, float accel[3]) {
    log_destination(log, destination);
    log_accel(log, accel);
    log_time_target(log, time_target);
}

// need the ifndef here so that we can ignore this code when compiling
// the firmware tests
#ifndef FWTEST
/**
 * Initializes the move primitive.
 *
 * This function runs once at system startup.
 */
static void move_init(void) 
{
	// Currently has nothing to do
}

/**
 * Starts a movement of this type.
 *
 * This function runs each time the host computer requests to start a move
 * movement.
 *
 * @param params the movement parameters, which are only valid until this
 * function returns and must be copied into this module if needed
 * @return void
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
	rotate(minor_vec, M_PI / 2);
}

/**
 * Ends a movement of this type.
 *
 * This function runs when the host computer requests a new movement while a
 * move movement is already in progress.
 * 
 * @return void
 */
static void move_end(void) 
{
}


/**
 * Ticks a movement of this type.
 *
 * This function runs at the system tick rate while this primitive is active.
 *
 * @param log the log record to fill with information about the tick, or
 * NULL if no record is to be filled
 * @return void 
 */
static void move_tick(log_record_t *log) {
	// get the state of the bot
	dr_data_t current_states;
	dr_get(&current_states);
	// setup the PhysBot data container
	PhysBot pb = setup_bot(current_states, destination, major_vec, minor_vec);
	// choose a wheel axis to rotate onto
	choose_rotation_destination(&pb, current_states.angle);
	// plan major axis movement
	float max_major_a = 3.0;
	float max_major_v = 3.0;
	float major_params[3] = {end_speed, max_major_a, max_major_v};
	plan_move(&pb.maj, major_params);
	// plan minor axis movement
	float max_minor_a = 1.5;
	float max_minor_v = 1.5;
	float minor_params[3] = {0, max_minor_a, max_minor_v};
	plan_move(&pb.min, minor_params);
	// plan rotation movement
	plan_move_rotation(&pb, current_states.avel);

	float accel[3] = {0, 0, pb.rot.accel};
	// rotate the accel and apply it
	to_local_coords(accel, pb, current_states.angle, major_vec, minor_vec);
	apply_accel(accel, accel[2]); 

    if (log) { move_to_log(log, pb.rot.time, accel); }

}

/**
 * The move movement primitive.
 */
const primitive_t MOVE_PRIMITIVE = {
	.direct = false,
 	.init = &move_init,
	.start = &move_start,
	.end = &move_end,
	.tick = &move_tick,
};

#endif
