#include "move.h"
#include "../bangbang.h"
#include "../control.h"
#include "../dr.h"
#include "../physics.h"
#include <math.h>
#include <stdio.h>


#define TIME_HORIZON 0.05f //s

//Parameters
static float destination[3], final_velocity[2];
static int counts, counts_passed;


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
  
	float scalar_speed;
	float init_position[2];
	dr_data_t current_states;

	printf ("move_start\r\n");
	// Convert into m/s and rad/s because physics is in m and s
	destination[0] = ((float)(params->params[0])/1000.0f);
	destination[1] = ((float)(params->params[1])/1000.0f);
	destination[2] = ((float)(params->params[2])/100.0f);
	scalar_speed = ((float)(params->params[3])/1000.0f);
 
	
	printf("input speed= %f", scalar_speed);	
#warning magic constant should use tick time constant here
	counts = (int)(params->params[3]/5.0f);
	counts_passed = 0;

	dr_get(&current_states);
	init_position[0] = current_states.x;
	init_position[1] = current_states.y;
   
	//decomposes speed into final velocity vector (modifies final_velocity param)
	decompose_radial(scalar_speed, final_velocity, init_position, destination); 	
	printf("final xVel= %f", final_velocity[0]);	
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
	rotate(vel, -current_states.angle); //put current vel into local coords

	float relative_destination[3];

	relative_destination[0] = destination[0] - current_states.x;
	relative_destination[1] = destination[1] - current_states.y;
	rotate(relative_destination, -current_states.angle); //put relative dest into local coords

	float relative_final_velocity[2] = {final_velocity[0], final_velocity[1]}; // copy by value not reference 
	rotate(final_velocity, -current_states.angle); // put relative final velocity into local coords

	float dest_angle = destination[2];
	float cur_angle = current_states.angle;

	// Pick whether to go clockwise or counterclockwise (based on smallest angle)
	if(dest_angle >= cur_angle ){                                                                                                        
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
	}                                                                                                                       

	float accel[3];

	const float relative_tick_start[2] = {0.0f, 0.0f};
	BBProfile r_profile;
	float radial_dist = 
		sqrtf(relative_destination[0]*relative_destination[0] + 
		relative_destination[1]*relative_destination[1]);
	float radial_vel = (vel[0]*relative_destination[0] + vel[1]*relative_destination[1])/radial_dist;
	PrepareBBTrajectoryMaxV(&r_profile, radial_dist, radial_vel,
		0, MAX_R_A, MAX_R_V); 
	PlanBBTrajectory(&r_profile);

	printf("\n\nt1= %f", r_profile.t1);	
	printf("t2= %f", r_profile.t2);	
	printf("t3= %f", r_profile.t3);	

	float radial_accel = BBComputeAvgAccel(&r_profile, TIME_HORIZON);
	float time_r = GetBBTime(&r_profile);

	decompose_radial(radial_accel, accel, relative_tick_start, 
		relative_destination); 	

	float deltaD = relative_destination[2];
	float timeTarget = (time_r > TIME_HORIZON) ? time_r : TIME_HORIZON;
	
	float targetVel = deltaD/timeTarget; 
	accel[2] = (targetVel - vel[2])/TIME_HORIZON;
	Clamp(&accel[2], MAX_T_A);

	if (log) {
		log->tick.primitive_data[0] = destination[0];//accel[0];
		log->tick.primitive_data[1] = destination[1];//accel[1];
		log->tick.primitive_data[2] = destination[2];//accel[2];
		log->tick.primitive_data[3] = time_r;//timeX;
		log->tick.primitive_data[4] = radial_accel;//timeY;
		log->tick.primitive_data[5] = timeTarget;
		log->tick.primitive_data[6] = deltaD;
		log->tick.primitive_data[7] = targetVel;
	}
	printf("x accel= %f", accel[0]);	
	printf("y accel= %f", accel[1]);	
	printf("theta accel= %f", accel[2]);	
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
