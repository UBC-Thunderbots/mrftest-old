#include "control.h"
#include "simulate.h"
//#include "dribbler.h"
//#include "leds.h"
#include "primitive.h"
#include "physics.h"
#include "bangbang.h"
#include <math.h>
#include <stdio.h>

// these are set to decouple the 3 axis from each other
// the idea is to clamp the maximum velocity and acceleration
// so that the axes would never have to compete for resources
#define TIME_HORIZON 0.05f //s

//TODO: find out actual wheel angles
static const float PREFERRED_DRIVE_ANGLES[4] = {M_PI*35.0/180.0, M_PI*155.0/180.0, M_PI*215.0/180.0, M_PI*325.0/180.0};

static float dest[3], end_speed, major_vec[2], minor_vec[2], major_angle;
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
void move_start(const primitive_params_t *params) 
{
	//Parameters: 	dest_x [mm]
	//				dest_y [mm]
	//				dest_ang [centi-rad]
	//				end_speed [millimeter/s]
  
	// Convert into m/s and rad/s because physics is in m and s
	dest[0] = ((float)(params->params[0])/1000.0f);
	dest[1] = ((float)(params->params[1])/1000.0f);
	dest[2] = ((float)(params->params[2])/100.0f);
	end_speed = ((float)(params->params[3])/1000.0f);
	
	dr_data_t state;
	dr_get(&state);
	
	float distance = norm2(dest[0] - state.x, dest[1] - state.y);	
	major_vec[0] = (dest[0] - state.x)/distance; 
	major_vec[1] = (dest[1] - state.y)/distance;
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
void move_tick() {
	//TODO: what would you like to log?

	dr_data_t state;
	dr_get(&state);

	float vel[3] = {state.vx, state.vy, state.avel};

	float relative_dest[3];

	relative_dest[0] = dest[0] - state.x;
	relative_dest[1] = dest[1] - state.y;
	relative_dest[2] = min_angle_delta(state.angle, dest[2]);
	

	/*if(norm2(relative_dest[0], relative_dest[1]) < 0.3){
		relative_dest[2] = min_angle_delta(state.angle, dest[2]);
	}else{
		//Want to lock onto a track where the robot wheels point in the direction of travel
		int i;
		float best_angle = dest[2];
		float test_angle;
		float delta;
		float min_delta = 99999.9;
		for(i=0;i++;i<4){
			test_angle = major_angle + PREFERRED_DRIVE_ANGLES[i];
			delta = abs(min_angle_delta(test_angle, dest[2]));
			if(delta < min_delta){
				best_angle = test_angle;
				min_delta = delta;	
			}
		}
		relative_dest[2] = best_angle;
	}*/


	BBProfile major_profile;
	BBProfile minor_profile;
	
	float major_disp = relative_dest[0]*major_vec[0] + relative_dest[1]*major_vec[1]; 
	float minor_disp = minor_vec[0]*relative_dest[0] + minor_vec[1]*relative_dest[1];
	
	//TODO: tune further: experimental
	float max_major_a = 3.0;//(get_var(0x00)/4.0);
	float max_major_v = 3.0;//(get_var(0x01)/4.0);
	float major_vel = major_vec[0]*vel[0] + major_vec[1]*vel[1];
	PrepareBBTrajectoryMaxV(&major_profile, major_disp, major_vel, end_speed, max_major_a, max_major_v); //3.5, 3.0
	PlanBBTrajectory(&major_profile);
	float major_accel = BBComputeAvgAccel(&major_profile, TIME_HORIZON);
	float time_major = GetBBTime(&major_profile);


	float max_minor_a = 1.5;//(get_var(0x02)/4.0);
	float max_minor_v = 1.5;//(get_var(0x03)/4.0);
	float minor_vel = minor_vec[0]*vel[0] + minor_vec[1]*vel[1];
	PrepareBBTrajectoryMaxV(&minor_profile, minor_disp, minor_vel, 0, max_minor_a, max_minor_v); //1.5, 1.5
	PlanBBTrajectory(&minor_profile);
	float minor_accel = BBComputeAvgAccel(&minor_profile, TIME_HORIZON);
	float time_minor = GetBBTime(&minor_profile);

	float timeTarget = (time_major > TIME_HORIZON) ? time_major : TIME_HORIZON;
	if(timeTarget > 0.5){
		timeTarget = 0.5;
	}	

	float accel[3] = {0};

	float targetVel = relative_dest[2]/timeTarget; 
	accel[2] = (targetVel - vel[2])/TIME_HORIZON;
	Clamp(&accel[2], MAX_T_A);


	float local_x_norm_vec[2] = {cosf(state.angle), sinf(state.angle)}; 
	float local_y_norm_vec[2] = {cosf(state.angle + M_PI/2), sinf(state.angle + M_PI/2)}; 
	
	//project accel into robot reference frame
	accel[0] = minor_accel*(local_x_norm_vec[0]*minor_vec[0] + local_x_norm_vec[1]*minor_vec[1] );
	accel[0] += major_accel*(local_x_norm_vec[0]*major_vec[0] + local_x_norm_vec[1]*major_vec[1] );
	accel[1] = minor_accel*(local_y_norm_vec[0]*minor_vec[0] + local_y_norm_vec[1]*minor_vec[1] );
	accel[1] += major_accel*(local_y_norm_vec[0]*major_vec[0] + local_y_norm_vec[1]*major_vec[1] );

	apply_accel(accel, accel[2]); // accel is already in local coords

}

