#include "catch.h"
#include "../bangbang.h"
#include "../control.h"
#include "../dr.h"
#include "../physics.h"
#include <math.h>
#ifndef FWSIM
#include <unused.h>
#endif // FWSIM

#define CATCH_MAX_X_V (MAX_X_V/2)
#define CATCH_MAX_Y_V (MAX_Y_V/2)
#define CATCH_MAX_T_V (MAX_T_V/2)

#define CATCH_MAX_X_A (2.0f)
#define CATCH_MAX_Y_A (2.0f)
#define CATCH_MAX_T_A (20.0f)

#define X_SPACE_FACTOR (0.002f)

#define TIME_HORIZON (0.01f)
#define MIN_SAFETY_DIST (0.04f)
#define STATIONARY_VEL_MAX (0.05f)

static primitive_params_t catch_param;

static float line_limit[3];

/**
	* \brief projects onto x axis angle radians CCW (dot product), note that
	* CCW projection is CW rotation
	*
	* \param[in] vector to project
	* \param[in] angle to rotate
 */
static float project_x_axis(float vector[2], float angle) {
	return cosf(angle)*vector[0] + sinf(angle)*vector[1];
}
				
/**
	* \brief projects onto y axis angle radians CCW (dot product), note that
	* CCW projection is CW rotation
	*
	* \param[in] vector to project
	* \param[in] angle to rotate
 */
static float project_y_axis(float vector[2], float angle) {
	return -sinf(angle)*vector[0] + cosf(angle)*vector[1];
}

/**
 * \brief Initializes the catch primitive.
 *
 * This function runs once at system startup.
 */
static void catch_init(void) {
}

/**
 * \brief Starts a movement of this type.
 *
 * This function runs each time the host computer requests to start a catch
 * movement.
 *
 * \param[in] params the movement parameters, which are only valid until this
 * function returns and must be copied into this module if needed
 */
static void catch_start(const primitive_params_t *params) {        
		for( unsigned int i = 0; i < 4; i++ ){
						catch_param.params[i] = params->params[i];      
		}
		catch_param.slow = params->slow;
		catch_param.extra = params->extra;
 
		// line not to be crossed with the ball
		// line is parameterized by [intercept.x, intercept.y, global orientation]
		line_limit[0] = 0; //(float)catch_param.params[1]/1000.0f;
		line_limit[1] = 0; //(float)catch_param.params[1]/1000.0f;
		line_limit[2] = 0; //(float)catch_param.params[1]/1000.0f;
}

/**
 * \brief Ends a movement of this type.
 *
 * This function runs when the host computer requests a new movement while a
 * catch movement is already in progress.
 */
static void catch_end(void) {
}

/**
 * \brief Ticks a movement of this type.
 *
 * This function runs at the system tick rate while this primitive is active.
 *
 * \param[out] log the log record to fill with information about the tick, or
 * \c NULL if no record is to be filled
 */
static void catch_tick(log_record_t *log) {

    // Grab variables	
	dr_data_t current_states;
	dr_get(&current_states);
#ifndef FWSIM // TODO: figure out why this is causing problem for FWSIM
	dr_ball_data_t current_ball_states;
	dr_get_ball(&current_ball_states);
#endif

    //TODO: actually update this
    float ball_vel[2] = {0.0,0.0};
    float ball_pos[2] = {-1,-1};
	float vel[3] = {current_states.vx, current_states.vy, current_states.avel};
    float pos[3] = {current_states.x, current_states.y, current_states.angle};

    float major_vec[2];
    float minor_vec[2];

    // Check if the ball is moving fast
	float ball_vel_norm = norm2(ball_vel[0], ball_vel[1]);
	if ( ball_vel_norm > STATIONARY_VEL_MAX) {
        // the ball is actually moving fast
        // we have to go onto the velocity line
        // so major is actually velocity line 
        major_vec[0] = ball_vel[0]/ball_vel_norm; 
        major_vec[1] = ball_vel[1]/ball_vel_norm; 
	}
	else{
        //otherwise use our vector to the ball as the the velocity line
        major_vec[0] = (ball_pos[0]-pos[0]); 
        major_vec[1] = (ball_pos[1]-pos[1]); 
        // normalize
        float major_vec_norm = norm2(major_vec[0], major_vec[1]);
        major_vec[0] /= major_vec_norm;
        major_vec[1] /= major_vec_norm;
	}

    minor_vec[0] = major_vec[0];
	minor_vec[1] = major_vec[1];
	rotate(minor_vec, M_PI/2);	

		
    // plan getting onto the minor with 0 end vel
	float minor_vel_cur = minor_vec[0]*vel[0] + minor_vec[1]*vel[1];

    // to get minor displacement, project the vector from robot to ball onto the minor axis 
    float minor_disp_cur = minor_vec[0]*(ball_pos[0]-pos[0]) + minor_vec[1]*(ball_pos[1]-pos[1]);

	BBProfile minor_profile;
	PrepareBBTrajectoryMaxV(&minor_profile, minor_disp_cur, minor_vel_cur, 0, MAX_X_A, MAX_X_V); 
	PlanBBTrajectory(&minor_profile);
	float minor_accel = BBComputeAvgAccel(&minor_profile, TIME_HORIZON);
	float time_minor = GetBBTime(&minor_profile);
    // how long it would take to get onto the velocity line with 0 minor vel
	float timeTarget = (time_minor > TIME_HORIZON) ? time_minor : TIME_HORIZON;
	

    // now calculate where we would want to end up intercepting the ball
    // predict where the ball would be by that time 
    float ball_pos_proj[2] = {ball_pos[0]+ball_vel[0]*timeTarget, ball_pos[1]+ball_vel[1]*timeTarget};  
    
    // get our major axis distance from where the ball would be by the time we get to the velocity line
    float major_disp_proj = major_vec[0]*(ball_pos_proj[0]-pos[0]) + major_vec[1]*(ball_pos_proj[1]-pos[1]);
    
    // TODO: handle the line constraint, have some fail condition here in case there is no way we can get there in time
    
    // calculate the position along the major axis where we want to catch the ball
    // aim for the exact position with a bit of breathing room
    // TODO: add an extra dynamic safety distance based on how much we want to accelerate in major axis
    float major_disp_intercept = major_disp_proj + MIN_SAFETY_DIST;  
    // desired end interception velocity
    float major_catch_vel = 0.1f;
    float major_vel_intercept = ball_vel_norm - major_catch_vel; 

	//TODO: tune further: experimental
	float major_vel = major_vec[0]*vel[0] + major_vec[1]*vel[1];
	BBProfile major_profile;
	PrepareBBTrajectoryMaxV(&major_profile, major_disp_intercept, major_vel, major_vel_intercept, CATCH_MAX_X_V, CATCH_MAX_X_A); 
	PlanBBTrajectory(&major_profile);
	float major_accel = BBComputeAvgAccel(&major_profile, TIME_HORIZON);
	float time_major = GetBBTime(&major_profile);
    
    // TODO: check time_major that the time_major we have doesn't exceed timeTarget


    // compute final accelerations to apply
	float accel[3] = {0};
    
    // get robot local coordinates
	float local_x_norm_vec[2] = {cosf(current_states.angle), sinf(current_states.angle)}; 
	float local_y_norm_vec[2] = {cosf(current_states.angle + M_PI/2), sinf(current_states.angle + M_PI/2)}; 

    // rotate acceleration onto robot local coordinates
	accel[0] = minor_accel*(local_x_norm_vec[0]*minor_vec[0] + local_x_norm_vec[1]*minor_vec[1] );
	accel[0] += major_accel*(local_x_norm_vec[0]*major_vec[0] + local_x_norm_vec[1]*major_vec[1] );
	accel[1] = minor_accel*(local_y_norm_vec[0]*minor_vec[0] + local_y_norm_vec[1]*minor_vec[1] );
	accel[1] += major_accel*(local_y_norm_vec[0]*major_vec[0] + local_y_norm_vec[1]*major_vec[1] );


    // Calculate the angular acceleration
    float major_angle = atan2f(major_vec[1], major_vec[0]);
    // robot must face opposite of ball velocity
	float angle_disp = min_angle_delta(pos[2], major_angle + M_PI);

    // orientate the robot in half the time
	float targetVel = 2*angle_disp/timeTarget; 
	accel[2] = (targetVel - vel[2])/timeTarget;
	Clamp(&accel[2], MAX_T_A);

    // normalize to acceleration 
    /*
    float len_accel = sqrtf((accel[0] * accel[0])+(accel[1] * accel[1]));
    accel[0] = accel[0]/len_accel; 
    accel[1] = accel[1]/len_accel;
    */

    /*
	if (log) {
		log->tick.primitive_data[0] = destination[0];//accel[0];
		log->tick.primitive_data[1] = destination[1];//accel[1];
		log->tick.primitive_data[2] = destination[2];//accel[2];
		log->tick.primitive_data[3] = accel[0];//timeX;
		log->tick.primitive_data[4] = accel[1];//timeY;
		log->tick.primitive_data[5] = accel[2];
		log->tick.primitive_data[6] = timeTarget;
	}
    */
    
    apply_accel(accel, accel[2]); // accel is already in local coords
}


/**
 * \brief The catch movement primitive.
 */
const primitive_t CATCH_PRIMITIVE = {
	.direct = false,
	.init = &catch_init,
	.start = &catch_start,
	.end = &catch_end,
	.tick = &catch_tick,
};
