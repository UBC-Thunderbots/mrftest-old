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
	destination[0] = ((float)(params->params[0])/1000.0f);
	destination[1] = ((float)(params->params[1])/1000.0f);
	destination[2] = ((float)(params->params[2])/100.0f);
	

	major_vec[0] = cosf(destination[2]); 
	major_vec[1] = sinf(destination[2]);
	minor_vec[0] = major_vec[0];
	minor_vec[1] = major_vec[1];
	rotate(minor_vec, M_PI/2);	

	// arm the chicker
	chicker_auto_arm((params->extra & 1) ? CHICKER_CHIP : CHICKER_KICK, params->params[3]);
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

	dr_data_t current_states;
	dr_get(&current_states);

	float vel[3] = {current_states.vx, current_states.vy, current_states.avel};

	float relative_destination[3];

	relative_destination[0] = destination[0] - current_states.x;
	relative_destination[1] = destination[1] - current_states.y;


	// Pick whether to go clockwise or counterclockwise (based on smallest angle)
	/*if(dest_angle >= cur_angle ){                                                                                                       
		float dest_sub = dest_angle - 2*M_PI;                                                                                        
		if((dest_sub - cur_angle)*(dest_sub - cur_angle) <= (dest_angle - cur_angle)*(dest_angle - cur_angle)){                             
			relative_destination[2] = dest_sub - cur_angle;                                                              
		}else{                                                                                                                 
			relative_destination[2] = dest_angle - cur_angle;                                                                                 
		}                                                                                                                      
	}else{                                                                                                                       
		float dest_plus = dest_angle + 2*M_PI;                                                                                       
		if((dest_plus - cur_angle)*(dest_plus - cur_angle) <= (dest_angle - cur_angle)*(dest_angle - cur_angle)){                           
			relative_destination[2] = dest_plus - cur_angle;                                                              
		}else{                                                                                                                 
			relative_destination[2] = dest_angle - cur_angle;                                                                                 
		}                                                                                                                      
	} */                                                                                                                      
	relative_destination[2] = min_angle_delta(current_states.angle, destination[2]);
	//relative_destination[2] =  destination[2] - current_states.angle;

	BBProfile major_profile;
	BBProfile minor_profile;
	
	float major_disp = relative_destination[0]*major_vec[0] + relative_destination[1]*major_vec[1]; 
	float minor_disp = minor_vec[0]*relative_destination[0] + minor_vec[1]*relative_destination[1];
	
	//TODO: tune further: experimental
	float major_vel = major_vec[0]*vel[0] + major_vec[1]*vel[1];
	PrepareBBTrajectoryMaxV(&major_profile, major_disp, major_vel, 0.2, 0.85, 0.85); 
	PlanBBTrajectory(&major_profile);
	float major_accel = BBComputeAvgAccel(&major_profile, TIME_HORIZON);
	float time_major = GetBBTime(&major_profile);

	float minor_vel = minor_vec[0]*vel[0] + minor_vec[1]*vel[1];
	PrepareBBTrajectoryMaxV(&minor_profile, minor_disp, minor_vel, 0, MAX_X_A, MAX_X_V); 
	PlanBBTrajectory(&minor_profile);
	float minor_accel = BBComputeAvgAccel(&minor_profile, TIME_HORIZON);
	float time_minor = GetBBTime(&minor_profile);

	float timeTarget = (time_minor > TIME_HORIZON) ? time_minor : TIME_HORIZON;
	
	float accel[3] = {0};

	float targetVel = 2*relative_destination[2]/timeTarget; 
	accel[2] = (targetVel - vel[2])/TIME_HORIZON;
	Clamp(&accel[2], MAX_T_A);

	if (log) {
		log->tick.primitive_data[0] = destination[0];//accel[0];
		log->tick.primitive_data[1] = destination[1];//accel[1];
		log->tick.primitive_data[2] = destination[2];//accel[2];
		log->tick.primitive_data[3] = accel[0];//timeX;
		log->tick.primitive_data[4] = accel[1];//timeY;
		log->tick.primitive_data[5] = accel[2];
		log->tick.primitive_data[6] = timeTarget;
	}

	float local_x_norm_vec[2] = {cosf(current_states.angle), sinf(current_states.angle)}; 
	float local_y_norm_vec[2] = {cosf(current_states.angle + M_PI/2), sinf(current_states.angle + M_PI/2)}; 
	
	accel[0] = minor_accel*(local_x_norm_vec[0]*minor_vec[0] + local_x_norm_vec[1]*minor_vec[1] );
	accel[0] += major_accel*(local_x_norm_vec[0]*major_vec[0] + local_x_norm_vec[1]*major_vec[1] );
	accel[1] = minor_accel*(local_y_norm_vec[0]*minor_vec[0] + local_y_norm_vec[1]*minor_vec[1] );
	accel[1] += major_accel*(local_y_norm_vec[0]*major_vec[0] + local_y_norm_vec[1]*major_vec[1] );

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

