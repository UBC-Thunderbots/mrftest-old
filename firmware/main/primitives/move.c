#include "../control.h"
#include "../physics.h"
#include "../bangbang.h"
#include <math.h>
#include <stdio.h>

#ifndef FWSIM
#include "shoot.h" // TODO: don't think this should be here, might be a mistake?
#include "../chicker.h"
#include "../dr.h"
#include "../dribbler.h"
#include "../leds.h"
#else
#include "move.h"
#include "../simulate.h"
#endif

// these are set to decouple the 3 axis from each other
// the idea is to clamp the maximum velocity and acceleration
// so that the axes would never have to compete for resources
#define TIME_HORIZON 0.05f //s

//TODO: find out actual wheel angles
static const float PREFERRED_DRIVE_ANGLES[4] = {M_PI*35.0/180.0, M_PI*155.0/180.0, M_PI*215.0/180.0, M_PI*325.0/180.0};

static float destination[3], end_speed, major_vec[2], minor_vec[2], major_angle;
// Only need two data points to form major axis vector.

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
	destination[0] = ((float)(params->params[0])/1000.0f);
	destination[1] = ((float)(params->params[1])/1000.0f);
	destination[2] = ((float)(params->params[2])/100.0f);
	end_speed = ((float)(params->params[3])/1000.0f);
	
	dr_data_t current_states;
	dr_get(&current_states);
	
	float distance = norm2(destination[0] - current_states.x, destination[1] - current_states.y);	
	major_vec[0] = (destination[0] - current_states.x)/distance; 
	major_vec[1] = (destination[1] - current_states.y)/distance;
	minor_vec[0] = major_vec[0];
	minor_vec[1] = major_vec[1];
	rotate(minor_vec, M_PI/2);	

	major_angle = atan2f(major_vec[1], major_vec[0]);
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
	//TODO: what would you like to log?

	dr_data_t current_states;
	dr_get(&current_states);

	float vel[3] = {current_states.vx, current_states.vy, current_states.avel};

	float relative_destination[3];

	relative_destination[0] = destination[0] - current_states.x;
	relative_destination[1] = destination[1] - current_states.y;
	relative_destination[2] = min_angle_delta(current_states.angle, destination[2]);
	

	/*if(norm2(relative_destination[0], relative_destination[1]) < 0.3){
		relative_destination[2] = min_angle_delta(current_states.angle, destination[2]);
	}else{
		//Want to lock onto a track where the robot wheels point in the direction of travel
		int i;
		float best_angle = destination[2];
		float test_angle;
		float delta;
		float min_delta = 99999.9;
		for(i=0;i++;i<4){
			test_angle = major_angle + PREFERRED_DRIVE_ANGLES[i];
			delta = abs(min_angle_delta(test_angle, destination[2]));
			if(delta < min_delta){
				best_angle = test_angle;
				min_delta = delta;	
			}
		}
		relative_destination[2] = best_angle;
	}*/


	BBProfile major_profile;
	BBProfile minor_profile;
	
	float major_disp = relative_destination[0]*major_vec[0] + relative_destination[1]*major_vec[1]; 
	float minor_disp = minor_vec[0]*relative_destination[0] + minor_vec[1]*relative_destination[1];
	
	//TODO: tune further: experimental
	float max_major_a = 3.0;//(get_var(0x00)/4.0);
	float max_major_v = 3.0;//(get_var(0x01)/4.0);
	float major_vel = major_vec[0]*vel[0] + major_vec[1]*vel[1];
	PrepareBBTrajectoryMaxV(&major_profile, major_disp, major_vel, end_speed, max_major_a, max_major_v); //3.5, 3.0
	//PrepareBBTrajectoryMaxV(&major_profile, major_disp, major_vel, end_speed, 3.5, 3.0); //3.5, 3.0
	PlanBBTrajectory(&major_profile);
	float major_accel = BBComputeAvgAccel(&major_profile, TIME_HORIZON);
	float time_major = GetBBTime(&major_profile);


	float max_minor_a = 1.5;//(get_var(0x02)/4.0);
	float max_minor_v = 1.5;//(get_var(0x03)/4.0);
	printf("max_minor_a: %f", max_minor_a);
	float minor_vel = minor_vec[0]*vel[0] + minor_vec[1]*vel[1];
	//PrepareBBTrajectoryMaxV(&minor_profile, minor_disp, minor_vel, 0, 1.5, 1.5); //1.5, 1.5
	PrepareBBTrajectoryMaxV(&minor_profile, minor_disp, minor_vel, 0, max_minor_a, max_minor_v); //1.5, 1.5
	PlanBBTrajectory(&minor_profile);
	float minor_accel = BBComputeAvgAccel(&minor_profile, TIME_HORIZON);
	float time_minor = GetBBTime(&minor_profile);

	float timeTarget = (time_major > TIME_HORIZON) ? time_major : TIME_HORIZON;
	if(timeTarget > 0.5){
		timeTarget = 0.5;
	}	

	float accel[3] = {0};

	float targetVel = relative_destination[2]/timeTarget; 
	accel[2] = (targetVel - vel[2])/TIME_HORIZON;
	Clamp(&accel[2], MAX_T_A);

#ifndef FWSIM
	if (log) {
		log->tick.primitive_data[0] = destination[0];//accel[0];
		log->tick.primitive_data[1] = destination[1];//accel[1];
		log->tick.primitive_data[2] = destination[2];//accel[2];
		log->tick.primitive_data[3] = accel[0];//timeX;
		log->tick.primitive_data[4] = accel[1];//timeY;
		log->tick.primitive_data[5] = accel[2];
		log->tick.primitive_data[6] = timeTarget;
	}
#endif

	float local_x_norm_vec[2] = {cosf(current_states.angle), sinf(current_states.angle)}; 
	float local_y_norm_vec[2] = {cosf(current_states.angle + M_PI/2), sinf(current_states.angle + M_PI/2)}; 
	
	accel[0] = minor_accel*(local_x_norm_vec[0]*minor_vec[0] + local_x_norm_vec[1]*minor_vec[1] );
	accel[0] += major_accel*(local_x_norm_vec[0]*major_vec[0] + local_x_norm_vec[1]*major_vec[1] );
	accel[1] = minor_accel*(local_y_norm_vec[0]*minor_vec[0] + local_y_norm_vec[1]*minor_vec[1] );
	accel[1] += major_accel*(local_y_norm_vec[0]*major_vec[0] + local_y_norm_vec[1]*major_vec[1] );

	apply_accel(accel, accel[2]); // accel is already in local coords

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


