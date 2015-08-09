#include "spin.h"
#include "../bangbang.h"
#include "../control.h"
#include "../dr.h"
#include "../physics.h"
#include <math.h>
#include <stdio.h>

#define TIME_HORIZON 0.5f

static float x_dest;
static float y_dest;
static float avel_final;
static bool slow;

/**
 * \brief Initializes the spin primitive.
 *
 * This function runs once at system startup.
 */
static void spin_init(void) {
}

/**
 * \brief Starts a movement of this type.
 *
 * This function runs each time the host computer requests to start a spin
 * movement.
 *
 * \param[in] params the movement parameters, which are only valid until this
 * function returns and must be copied into this module if needed
 */

// 0th is x (mm), 1st is y (mm), 2nd is angular velocity in centirad/s
// input to 3->4 matrix is quarter-degrees per 5 ms, matrix is dimensionless
// linear ramp up for velocity and linear fall as robot approaches point
// constant angular velocity

static void spin_start(const primitive_params_t *params) {
	x_dest = (float)(params->params[0]/1000.0f);
	y_dest = (float)(params->params[1]/1000.0f);
	avel_final = (float)(params->params[2]/100.0f);
	slow = params->slow;	
}

/**
 * \brief Ends a movement of this type.
 *
 * This function runs when the host computer requests a new movement while a
 * spin movement is already in progress.
 */
static void spin_end(void) {
}

/**
 * \brief Ticks a movement of this type.
 *
 * This function runs at the system tick rate while this primitive is active.
 *
 * \param[out] log the log record to fill with information about the tick, or
 * \c NULL if no record is to be filled
 */

static void spin_tick(log_record_t *log) {

	dr_data_t data;

	BBProfile x_trajectory;
	BBProfile y_trajectory;	

	// These are current values.
	float v_max[3];
	float a_max[3];
	float coords[3];
	float velocities[3];

	// These are values to set.
	float accel[3];

	dr_get(&data);
	coords[0] = data.x;
	coords[1] = data.y;
	coords[2] = data.angle;
	velocities[0] = data.vx;
	velocities[1] = data.vy;
	velocities[2] = data.avel;

	// Temporarily use constants

	v_max[0] = MAX_X_V;
	v_max[1] = MAX_Y_V;
	v_max[2] = MAX_T_V;

	a_max[0] = MAX_X_A;
	a_max[1] = MAX_Y_A;

	log->tick.primitive_data[0] = coords[0];
	log->tick.primitive_data[1] = coords[1];
	log->tick.primitive_data[2] = coords[2];
	
	log->tick.primitive_data[3] = velocities[0];
	log->tick.primitive_data[4] = velocities[1];
	log->tick.primitive_data[5] = velocities[2];

	PrepareBBTrajectory(&x_trajectory, x_dest-coords[0], velocities[0], a_max[0]);
	PrepareBBTrajectory(&y_trajectory, y_dest-coords[1], velocities[1], a_max[1]);

	PlanBBTrajectory(&x_trajectory);
	PlanBBTrajectory(&y_trajectory);
	
	accel[0] = BBComputeAccel(&x_trajectory, TIME_HORIZON);
	accel[1] = BBComputeAccel(&y_trajectory, TIME_HORIZON);
	accel[2] = (avel_final-velocities[2])/0.05f;
	
	log->tick.primitive_data[6] = accel[0];
	log->tick.primitive_data[7] = accel[1];
	log->tick.primitive_data[8] = accel[2];


	if(accel[2] >  MAX_T_A) {
		accel[2] = MAX_T_A;
	}
	
	if (accel[2] < -MAX_T_A) {
		accel[2] = -MAX_T_A;
	}
	
	rotate(accel, -coords[2]);
	apply_accel(accel, accel[2]);
}

/**
 * \brief The spin movement primitive.
 */
const primitive_t SPIN_PRIMITIVE = {
	.direct = false,
	.init = &spin_init,
	.start = &spin_start,
	.end = &spin_end,
	.tick = &spin_tick,
};


void tick_accel(float coord, float dest, float v, float v_max, float v_step, float a_max, float *accel) { 
/*
	printf("Current  : %f\n", coord);
	printf("Dest  :    %f\n", dest);
	printf("Velo  :    %f\n", v);
	printf("V_MAX :    %f\n", v_max);
	printf("V_STEP:    %f\n", v_step);
	printf("A_MAX :    %f\n", a_max);
	printf("Accels:    %f %f %f\n", accel[0], accel[1], accel[2]);
*/
	if(fabsf(coord) < fabsf(dest/2)) {
		*accel = (v_max*v_max-v*v)/(2*((dest/2)-coord));
	}
	else if((fabsf(dest/2) <= fabsf(coord)) && (fabsf(coord) < fabsf(dest))) {
		*accel = (-v*v)/(2*(dest-coord+SPACE_FACTOR));
	}
	else if(fabsf(coord) > fabsf(dest)) {
		if(fabsf(v) > v_step) {
			if(v > 0) {
				*accel = -a_max;
			}
			else {
				*accel = a_max;
			}
		}
		else {
			*accel = 0;
		}
	}
}
	
