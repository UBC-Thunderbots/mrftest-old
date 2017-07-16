#include "move.h"
#include "../bangbang.h"
#include "../control.h"
#include "../dr.h"
#include "../physics.h"
#include <math.h>
#include <stdio.h>


#define TIME_HORIZON 0.05f //s

//Parameters
static float destination[3], final_velocities[2];
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
	//				time [ms]
  //        scalar vf [enum, 0-3]
  
  float scalar_speed;

	printf ("move_start\r\n");
	// Convert into m/s and rad/s because physics is in m and s
	destination[0] = ((float)(params->params[0])/1000.0f);
	destination[1] = ((float)(params->params[1])/1000.0f);
	destination[2] = ((float)(params->params[2])/100.0f);

	printf("x dest= %f", destination[0]);	
#warning magic constant should use tick time constant here
	counts = (int)(params->params[3]/5.0f);
	counts_passed = 0;

  scalar_speed = SCALAR_VF[params->params[4]]

  	
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
	rotate(vel, -current_states.angle);

	float relative_destination[3];

	relative_destination[0] = destination[0] - current_states.x;
	relative_destination[1] = destination[1] - current_states.y;
	rotate(relative_destination, -current_states.angle);
  rotate(final_velocities, -current_states.angle);

	float dest_angle = destination[2];
	float cur_angle = current_states.angle;

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

	float max_accel[3] = {MAX_X_A, MAX_Y_A, MAX_T_A};
	float accel[3];

	BBProfile Xprofile;
	PrepareBBTrajectory(&Xprofile, relative_destination[0], vel[0], max_accel[0]);
	PlanBBTrajectory(&Xprofile);
	accel[0] = BBComputeAvgAccel(&Xprofile, TIME_HORIZON);
	float timeX = GetBBTime(&Xprofile);

	BBProfile Yprofile;
	PrepareBBTrajectory(&Yprofile, relative_destination[1], vel[1], max_accel[1]);
	PlanBBTrajectory(&Yprofile);
	accel[1] = BBComputeAvgAccel(&Yprofile, TIME_HORIZON);
	float timeY = GetBBTime(&Yprofile);

	float deltaD = relative_destination[2];
	float timeTarget = (timeY > timeX)?timeY:timeX;
	if (timeX < TIME_HORIZON && timeY < TIME_HORIZON) {
		timeTarget = TIME_HORIZON;	
	}
	
	float targetVel = deltaD/timeTarget; 
	accel[2] = (targetVel - vel[2])/TIME_HORIZON;
	Clamp(&accel[2], MAX_T_A);

	if (log) {
	        log->tick.primitive_data[0] = destination[0];//accel[0];
	        log->tick.primitive_data[1] = destination[1];//accel[1];
	        log->tick.primitive_data[2] = destination[2];//accel[2];
		log->tick.primitive_data[3] = timeX;
		log->tick.primitive_data[4] = timeY;
		log->tick.primitive_data[5] = timeTarget;
		log->tick.primitive_data[6] = deltaD;
		log->tick.primitive_data[7] = targetVel;
	}

	//rotate(accel, -angle);//-current_states.angle);
	apply_accel(accel, accel[2]);
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
