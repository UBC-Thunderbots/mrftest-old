#include "catch.h"
#include "../bangbang.h"
#include "../control.h"
#include "../dr.h"
#include "../physics.h"
#include <math.h>
#include <unused.h>

#define CATCH_MAX_X_V (MAX_X_V/2)
#define CATCH_MAX_Y_V (MAX_Y_V/2)
#define CATCH_MAX_T_V (MAX_T_V/2)

#define CATCH_MAX_X_A (2.0f)
#define CATCH_MAX_Y_A (2.0f)
#define CATCH_MAX_T_A (20.0f)

#define X_SPACE_FACTOR (0.002f)

#define TIME_HORIZON (0.5f)

static primitive_params_t catch_param;

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
static void catch_tick(log_record_t *UNUSED(log)) {
	dr_data_t data;
  // The values on the dead reckoning axis
  float position[3];
  float velocity[3];

  // The values on the final axis
  float position_f[3];
  float velocity_f[3];
  float accel_f[3];

  // The variables for velocity control
  float x_vel_diff;
  float x_vel_abs;

  // Trajectory planning variables
  BBProfile y_trajectory;
  BBProfile t_trajectory;

  // The angle difference between final axis and dr axis
  float angle_final = (float)catch_param.params[0]/100.0f;
  // the vertical y displacement, after rotation (along final axis)
  float y_final = (float)catch_param.params[1]/1000.0f;
  // the horizontal velocity, after rotation (along final axis)
	float vx_final = (float)catch_param.params[2]/1000.0f;

  // grab position, velocity measurement
  dr_get(&data);
  position[0] = data.x;
  position[1] = data.y;
  position[2] = data.angle;
  velocity[0] = data.vx;
  velocity[1] = data.vy;
  velocity[2] = data.avel;

  // Calculate x-acceleration component along final axes
  // in order to reach final required speed.
  velocity_f[0] = project_x_axis(velocity, angle_final);
  x_vel_diff = vx_final - velocity_f[0];
  x_vel_abs = fabsf(x_vel_diff);
  if (x_vel_diff > x_vel_abs/2) {
    accel_f[0] = CATCH_MAX_X_A;
  }
  else if (x_vel_diff > X_SPACE_FACTOR) {
    accel_f[0] = CATCH_MAX_X_A/10;
  }
  else if (x_vel_diff < -x_vel_abs/2) {
    accel_f[0] = -CATCH_MAX_X_A;
  }
  else if (x_vel_diff < -X_SPACE_FACTOR) {
    accel_f[0] = -CATCH_MAX_X_A/10;
  }
  else {
    accel_f[0] = 0;
  }

  // Calculate y-acceleration component along final axes
  // in order to reach final required position. 
  position_f[1] = project_y_axis(position, angle_final);
  velocity_f[1] = project_y_axis(velocity, angle_final);

  PrepareBBTrajectory(&y_trajectory, y_final-position_f[1], velocity_f[1], CATCH_MAX_Y_A);
  PlanBBTrajectory(&y_trajectory);
  accel_f[1] = BBComputeAccel(&y_trajectory, TIME_HORIZON);

  // Calculate angular acceleration in order to reach final required angle.
  PrepareBBTrajectory(&t_trajectory, angle_final-position[2], velocity[2], CATCH_MAX_T_A);
  PlanBBTrajectory(&t_trajectory);
  accel_f[2] = BBComputeAccel(&t_trajectory, TIME_HORIZON);
  
  // Rotate final acceleration onto local coordinate axis
  rotate(accel_f, (angle_final-position[2]));
  apply_accel(accel_f, accel_f[2]);

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
