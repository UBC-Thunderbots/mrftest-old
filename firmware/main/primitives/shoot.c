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
#define SHOOT_TIME_HORIZON 0.05f //s

//static primitive_params_t shoot_param;

static float destination[3];
static float direction[2];

//static float compute_accel(float d_target, float d_cur, float v_cur, float v_max, float a_max);

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
	//printf("====entering shoot start=====\r\n");

	// Convert into m/s and rad/s because physics is in m and s
	destination[0] = ((float)(params->params[0])/1000.0f);
	destination[1] = ((float)(params->params[1])/1000.0f);
	destination[2] = ((float)(params->params[2])/100.0f);
	
	direction[0] = cos(destination[2]);
	direction[1] = sin(destination[2]);

	chicker_auto_arm((params->extra & 1) ? CHICKER_CHIP : CHICKER_KICK, params->params[3]);
	if (!(params->extra & 1)) {
		dribbler_set_speed(8000);
	}
	//printf("\r\n====leaving shoot start=====\r\n");
	
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
	float pos[3] = {current_states.x, current_states.y, current_states.angle};
	float max_accel[3] = {MAX_X_A, MAX_Y_A, MAX_T_A};
	float accel[3];

	float delta[2]  = {destination[0] - pos[0], destination[1] - pos[1]};

	float delta_major = delta[0]*direction[0] + delta[1]*direction[1]; //dot product
	float delta_minor = direction[0]*delta[1] - direction[1]*delta[0]; //cross product

	BBProfile majorProfile;
	//TODO: determine proper magic number for final velocity
	//TODO: reduce control strength along major axis
	PrepareBBTrajectory(&majorProfile, delta_major, vel[0]*direction[0] + vel[1]*direction[1], 0.7, max_accel[0]);
	PlanBBTrajectory(&majorProfile);
	accel[0] = BBComputeAvgAccel(&majorProfile, SHOOT_TIME_HORIZON);
	float timeMajor = GetBBTime(&majorProfile);

	BBProfile minorProfile;
	PrepareBBTrajectory(&minorProfile, delta_minor, direction[0]*vel[1] - direction[1]*vel[0], 0, max_accel[0]);
	PlanBBTrajectory(&minorProfile);
	accel[0] = BBComputeAvgAccel(&minorProfile, SHOOT_TIME_HORIZON);
	float timeMinor = GetBBTime(&majorProfile);


	float deltaD = destination[2] - pos[2];
	float timeTarget = (timeMajor > timeMinor)?timeMajor:timeMinor;
	if (timeMajor < SHOOT_TIME_HORIZON && timeMinor < SHOOT_TIME_HORIZON) {
		timeTarget = SHOOT_TIME_HORIZON;	
	}
	
	float targetVel = deltaD/timeTarget; 
	accel[2] = (targetVel - vel[2])/ SHOOT_TIME_HORIZON;
	Clamp(&accel[2], MAX_T_A);

	if (log) {
		log->tick.primitive_data[0] = accel[0];
		log->tick.primitive_data[1] = accel[1];
		log->tick.primitive_data[2] = accel[2];
		log->tick.primitive_data[3] = timeMajor;
		log->tick.primitive_data[4] = timeMinor;
		log->tick.primitive_data[5] = timeTarget;
		log->tick.primitive_data[6] = deltaD;
		log->tick.primitive_data[7] = targetVel;
	}

	rotate(accel, destination[2] - current_states.angle);
	apply_accel(accel, accel[2]);

	/*dr_data_t current_states;
	dr_get(&current_states);

	float vel[3] = {current_states.vx, current_states.vy, current_states.avel};
	float pos[3] = {current_states.x, current_states.y, current_states.angle};
	float max_accel[3] = {MAX_X_A, MAX_Y_A, MAX_T_A};

	float accel[3];

	uint8_t i;

	for (i = 0; i < 3; i++)
	{
		BBProfile profile;
		PrepareBBTrajectory(&profile, destination[i]-pos[i], vel[i], max_accel[i]);
		PlanBBTrajectory(&profile);
		float jerk = 0;
		accel[i] = BBComputeAccel(&profile, 0.5);
	}
	
	rotate(accel, -current_states.angle);
	apply_accel(accel, accel[2]);*/

	/*dr_data_t data;
	float vel_diff[3];
	float acc_target[3];

	static unsigned int frame = 0;
	
	// need to evaluate velocity threshold for each of the axis

	//leds_link_set(1);	

	// step 1. grab position, velocity measurement
	dr_get(&data);

	if(frame == 0){
		//printf("=== dr === %f, %f, %f, %f, %f, %f  \r\n", data.x, data.y, data.angle, data.vx, data.vy, data.avel);
	}


	acc_target[0] = compute_accel_track_pos_1D(shoot_param.params[0]/1000.0f, data.x, data.vx, SHOOT_MAX_X_V, SHOOT_MAX_X_A);
	acc_target[1] = compute_accel_track_pos_1D(shoot_param.params[1]/1000.0f, data.y, data.vy, SHOOT_MAX_Y_V, SHOOT_MAX_Y_A);
	acc_target[2] = compute_accel_track_pos_1D(shoot_param.params[2]/100.0f, data.angle, data.avel, SHOOT_MAX_T_V, SHOOT_MAX_T_A);

	
	if(frame == 0){
		//printf("=== acc_target === %f, %f, %f\r\n", acc_target[0], acc_target[1], acc_target[2]);
	}

	apply_accel(acc_target, acc_target[2]);
	
	// step n. logging stuff todo
	

	frame++;
	frame = frame%99;*/
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
/*
// assuming units are the same except by order of time
static float compute_accel(float d_target, float d_cur, float v_cur, float v_max, float a_max){
	// step 2. diff distination todo: check unit
	//pos_diff[0] = ((float)shoot_param.params[0]/1000.0f)-data.x;
	//pos_diff[1] = ((float)shoot_param.params[1]/1000.0f)-data.y;
	//pos_diff[2] = ((float)shoot_param.params[2]/100.0f)-data.angle;

	float d_diff, v_thresh_abs, v_target, v_diff, a_target;

	float d_hysteresis = a_max*0.005f;
	float v_hysteresis = a_max*0.05f;

	static unsigned int frame = 0;
	
	frame++;
	frame = frame%34;

	d_diff = d_target-d_cur;

	// step 3. get threshold velocity todo: check if we have sqrt routine, abs
	// todo: check if the math function only takes double... isn't really inefficient?
	v_thresh_abs = sqrtf(2*fabsf(d_diff)*a_max);


	// step 3.1 remember the vel_thresh_abs has no directional dependence 
	//	    this is the correction step
	// step 3.2 clamp thresh to maximum velocity
	// clamp and get the sign right
	if(v_thresh_abs > v_max) {
		if(d_diff > d_hysteresis ){ 
			v_target = v_max;
		} else if( d_diff < -d_hysteresis ){
			v_target = -v_max;
		} else {
			v_target = 0.0f;
		}
	} else{// if(vel_thresh_abs[i] > 0.01f) {
		if(d_diff > d_hysteresis ){
			v_target = v_thresh_abs;
		} else if( d_diff < -d_hysteresis ){
			v_target = -v_thresh_abs;
		} else {
			v_target = 0.0f;
		}
	} 

	// option 1: there is apparently a function to call to do velocity control
	
	// option 2: set max acceleration yourself
	
	v_diff = v_target-v_cur;

	// this set accel to maximum or zero
	if(v_diff > v_hysteresis ){	
		a_target = a_max;
	} else if(v_diff < -v_hysteresis ) {
		a_target = -a_max;
	} else {
		a_target = 0.0f;
	}

	
	if(frame == 0){
		//printf("=== compute_accel === %f, %f, %f, %f\r\n", d_diff, v_thresh_abs, v_target, v_diff);
	}

	return a_target;
}*/
