#include "spin.h"
#include "../../firmware/main/bangbang.h"
#include "../../firmware/main/control.h"
#include "../../firmware/main/simulate.h"
#include "../../firmware/main/physics.h"
#include <math.h>
#include <stdio.h>

#define TIME_HORIZON 0.5f

static float x_final;
static float y_final;
static float avel_final;
static bool slow;

static float major_vec[2];
static float minor_vec[2];
static float major_angle;

/**
 * \brief Starts a movement of this type.
 *
 * This function runs each time the host computer requests to start a spin
 * movement.
 *
 * \param[in] params the movement parameters, which are only valid until this
 * function returns and must be copied into this module if needed
 */
// input to 3->4 matrix is quarter-degrees per 5 ms, matrix is dimensionless
// linear ramp up for velocity and linear fall as robot approaches point
// constant angular velocity
void spin_start(primitive_params_t *p) {
	// Parameters:  param[0]: g_destination_x   [mm]
    //              param[1]: g_destination_y   [mm]
    //              param[2]: g_angular_v_final [centi-rad/s]
    //              extra:    g_end_speed       [millimeter/s]

    // Parse the parameters with the standard units
	x_final = (float)p->params[0] / 1000.0f;
    y_final = (float)p->params[1] / 1000.0f;
    avel_final = (float)p->params[2] / 100.0f;
    slow = p->slow;

    // Get robot current data
    dr_data_t now;
    dr_get(&now);

    // Construct major and minor axis for the path
    // get maginutude
    float distance = norm2(x_final-now.x, y_final-now.y);

    // major vector - unit vector from start to destination
    major_vec[0] = (x_final-now.x)/distance;
    major_vec[1] = (y_final-now.y)/distance;

    // minor vector - orthogonal to major vector
    minor_vec[0] = -major_vec[1];
    minor_vec[1] = major_vec[0];

    // major angle - angle relative to global x
    major_angle = atan2f(major_vec[1], major_vec[0]);
}

void spin_tick() {
dr_data_t now;
    dr_get(&now);

    // Trajectories
    BBProfile major;
    BBProfile minor;

    // current to destination vector
    float x_disp = x_final-now.x;
    float y_disp = y_final-now.y;

    // project current to destination vector to major/minor axis
    float major_disp = x_disp*major_vec[0] + y_disp*major_vec[1];
    float minor_disp = x_disp*minor_vec[0] + y_disp*minor_vec[1];

    // project velocity vector to major/minor axis
    float major_vel = now.vx*major_vec[0] + now.vy*major_vec[1];
    float minor_vel = now.vx*minor_vec[0] + now.vy*minor_vec[1];

    // Prepare trajectory
    PrepareBBTrajectoryMaxV(&major, major_disp, major_vel, 0, MAX_X_A, MAX_X_V);
    PrepareBBTrajectoryMaxV(&minor, minor_disp, minor_vel, 0, MAX_Y_A, MAX_Y_V);

    // Plan
    PlanBBTrajectory(&major);
    PlanBBTrajectory(&minor);

    // Compute acceleration
    float major_accel = BBComputeAccel(&major, TIME_HORIZON);
    float minor_accel = BBComputeAccel(&minor, TIME_HORIZON);
    float a_accel = (avel_final-now.avel) / 0.05f;

    // Clamp acceleration
    if (a_accel > MAX_T_A) {
        a_accel = MAX_T_A;
    }
    if (a_accel < -MAX_T_A) {
        a_accel = -MAX_T_A;
    }

    // Local cartesian represented as global cartesian
    float local_x_vec[2] = {
        cosf(now.angle), 
        sinf(now.angle)
    };
    float local_y_vec[2] = {
        cosf((now.angle + M_PI) / 2),
        sinf((now.angle + M_PI) / 2)
    };

    // Get local x acceleration
    float major_dot_x = major_vec[0]*local_x_vec[0] + major_vec[1]*local_x_vec[1];
    float minor_dot_x = minor_vec[0]*local_x_vec[0] + minor_vec[1]*local_x_vec[1];
    float x_accel = major_accel*major_dot_x + minor_accel*minor_dot_x;

    // Get local y acceleration
    float major_dot_y = major_vec[0]*local_y_vec[0] + major_vec[1]*local_y_vec[1];
    float minor_dot_y = minor_vec[0]*local_y_vec[0] + minor_vec[1]*local_y_vec[1];
    float y_accel = major_accel*major_dot_y + minor_accel*minor_dot_y;

    // Apply acceleration in robot's coordinates
    float distance = norm2(x_final-now.x, y_final-now.y);
    float linear_acc[2] = {
        x_accel,
        y_accel,
    };
    apply_accel(linear_acc, a_accel);
}
