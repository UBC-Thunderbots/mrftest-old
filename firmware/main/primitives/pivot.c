#include "pivot.h"
#include "../bangbang.h"
#include "../control.h"
#include "../dr.h"
#include "../physics.h"
#include <math.h>

//params from host

/**
 * \brief The rectangular vector from initial robot position to pivot centre.
 *
 * This is measured in metres.
 */
static float center[2];

/**
 * \brief The angle component of the polar vector from pivot centre to target
 * position.
 */
static float angle;

/**
 * \brief The amount by which the robot should rotate left or right form its
 * initial orientation.
 *
 * This is measured in radians.
 */
static float addRot;

/**
 * \brief The radius component of the polar vector from pivot centre to target
 * robot position, equivalently initial robot position, equivalently the length
 * of @ref center.
 *
 * This is measured in metres.
 */
static float rad;

/**
 * \brief The angle component of the polar vector from pivot centre to the most
 * recent robot position.
 *
 * This is measured in radians.
 */
static float prevTheta;

#define HORIZON 0.1f
#define RADIAL_ACCEL_MAX 3.0f
#define RADIAl_ACCEL_CLAMP 6.0f
#define ANGULAR_ACCEL_MAX 3.0f

/**
 * \brief Initializes the pivot primitive.
 *
 * This function runs once at system startup.
 */
static void pivot_init(void) {
}

/**
 * \brief Starts a movement of this type.
 *
 * This function runs each time the host computer requests to start a pivot
 * movement.
 *
 * \param[in] params the movement parameters, which are only valid until this
 * function returns and must be copied into this module if needed
 */
static void pivot_start(const primitive_params_t *params) {
	center[0] = params->params[0] / 1000.0;
	center[1] = params->params[1] / 1000.0;
	angle = params->params[2] / 100.0;
	addRot = params->params[3] / 100.0;
	rad = sqrtf(center[0]*center[0] + center[1]*center[1]);
	float pos[2];
	pos[0] = 0;
	pos[1] = 0;
	vectorSub(pos,center,2);
	Cart2Pol(pos); //working in polar coordinates because pivot
	angle += pos[1];
	prevTheta = pos[1];
}

/**
 * \brief Ends a movement of this type.
 *
 * This function runs when the host computer requests a new movement while a
 * pivot movement is already in progress.
 */
static void pivot_end(void) {
}

/**
 * \brief Ticks a movement of this type.
 *
 * This function runs at the system tick rate while this primitive is active.
 *
 * \param[out] log the log record to fill with information about the tick, or
 * \c NULL if no record is to be filled
 */
static void pivot_tick(log_record_t *log) {
	dr_data_t loc;
	dr_get(&loc);
	float pos[3]; 
	float vel[3];

	pos[0] = loc.x;
	pos[1] = loc.y;
	pos[2] = loc.angle;

	vel[0] = loc.vx;
	vel[1] = loc.vy;
	vel[2] = loc.avel;

	vectorSub(pos,center,2); //rebase pos to center of coord system (only x,y)
	Cart2Pol(pos); //working in polar coordinates because pivot
	float thetaDiff = pos[1] - prevTheta;
	if (thetaDiff > (float)M_PI) {
		thetaDiff -= 2.0f*(float)M_PI;
	} 

	if (thetaDiff < -(float)M_PI) {
		thetaDiff += 2.0f*(float)M_PI;
	}
	pos[1] = prevTheta + thetaDiff;
	prevTheta = pos[1]; 
	if (log) {
		log->tick.primitive_data[0] = pos[0];
		log->tick.primitive_data[1] = pos[1];
	}


	CartVel2Pol(pos, vel); //convert velocity to polar
	if (log) {
		log->tick.primitive_data[2] = vel[0];
		log->tick.primitive_data[3] = vel[1];
	}


#warning TODO: compute max acceleratons in the R,T domain
	float Pacc[2];
	BBProfile theta;
	float radial_acc_= RADIAL_ACCEL_MAX/rad;
	if( radial_acc_ >= 6.0f ){
		radial_acc_ = 6.0f;
	} else if ( radial_acc_ <= -6.0f ){
		radial_acc_ = -6.0f;
	}
	PrepareBBTrajectory(&theta, angle - pos[1], vel[1], radial_acc_);
	PlanBBTrajectory(&theta);
	Pacc[1] = BBComputeAvgAccel(&theta, HORIZON);
	float Ttime = GetBBTime(&theta);
	BBProfile radius;
	PrepareBBTrajectory(&radius, rad - pos[0], vel[0], RADIAL_ACCEL_MAX);
	PlanBBTrajectory(&radius);
	Pacc[0] = BBComputeAvgAccel(&radius, HORIZON);
	float Rtime = GetBBTime(&radius);
	if (log) {
		log->tick.primitive_data[4] = Pacc[0];
		log->tick.primitive_data[5] = Pacc[1];
	}

	float acc[2];
	PolAcc2Cart(pos, vel, Pacc, acc);
	if (log) {
		log->tick.primitive_data[6] = acc[0];
		log->tick.primitive_data[7] = acc[1];
	}
	BBProfile Rotation;
	PrepareBBTrajectory(&Rotation,addRot-pos[2],vel[2],MAX_T_A);
	PlanBBTrajectory(&Rotation);
	float As = BBComputeAvgAccel(&Rotation, HORIZON);
	if (log) {
		log->tick.primitive_data[8] = As;
	}

	rotate(acc, -pos[2]);
	apply_accel(acc,As);
}

/**
 * \brief The pivot movement primitive.
 */
const primitive_t PIVOT_PRIMITIVE = {
	.direct = false,
	.init = &pivot_init,
	.start = &pivot_start,
	.end = &pivot_end,
	.tick = &pivot_tick,
};
