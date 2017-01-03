#include "dr.h"
#include "dsp.h"
#include "encoder.h"
#include "kalman.h"
#include "physics.h"
#include "sensors.h"

#include <stdio.h>
#include <stdint.h>

static dr_data_t current_state;
static kalman_data_t kalman_data;
static bool is_calibrated;
static int32_t calibration_vals[3][CALIBRATION_LENGTH];
static int calibration_count;
static int32_t offset[3] = {0};




/**
 * \brief The most recent ball and camera data frames.
 */
static robot_camera_data_t robot_camera_data;
static ball_camera_data_t ball_camera_data;

/**
 * \brief called a system boot to configure deadreckoning system
 */
void dr_init(void) {
  is_calibrated = false;
  calibration_count = 0;

  robot_camera_data.x = 0;
  robot_camera_data.y = 0;
  robot_camera_data.angle = 0;
  robot_camera_data.timestamp = 0;
  
  ball_camera_data.x = 0;
  ball_camera_data.y = 0;
  ball_camera_data.timestamp = 0;

}


/**
 * \brief gets whether the dead reckoning system is calibrated
 */
bool dr_calibrated(void) {
  return is_calibrated;
}


/**
 * \brief resets the position to origin but preserves additional state
 */
void dr_reset(void) {
	current_state.x = 0.0f;
	current_state.y = 0.0f;
	current_state.angle = 0.0f;
  current_state.vx = 0.0f;
  current_state.vy = 0.0f;
  current_state.avel = 0.0f;

  kalman_data.cam_x = 0.0f;
  kalman_data.cam_y = 0.0f;
  kalman_data.cam_t = 0.0f;
  kalman_data.x_accel = 0.0f;
  kalman_data.y_accel = 0.0f;
  kalman_data.t_accel = 0.0f;
  kalman_data.accelerometer_x = 0.0f;
  kalman_data.accelerometer_y = 0.0f;
  kalman_data.accelerometer_z = 0.0f;
  kalman_data.gyro = 0.0f;
  kalman_data.wheels_x = 0.0f;
  kalman_data.wheels_y = 0.0f;
  kalman_data.wheels_t = 0.0f;
  kalman_data.new_camera_data = false;
}

/**
 * \brief called on tick after accel and gyros are read to update state
 *
 * \param[out] log the log record to fill, if any
 */
void dr_tick(log_record_t *log) {

  sensors_gyro_data_t gyrodata;
  sensors_accel_data_t acceldata;
  float robot_accels[3];
  float gyro_speed;
  int16_t accel_out[3] = {0};
  int drop_flag = 0;
	int i;
  int32_t calibration_acc[3] = {0};
  float encoder_speeds[4];
  float wheel_speeds[3];

  gyrodata = sensors_get_gyro();
  acceldata = sensors_get_accel();

  if(gyrodata.status) {
	  gyro_speed = MS_PER_GYRO*gyrodata.data.reading.z;
	}
  else {
    gyro_speed = current_state.avel*ROBOT_RADIUS;
  }

  if (acceldata.status) {
    accel_out[0] = acceldata.data.reading.x;
    accel_out[1] = acceldata.data.reading.y;
    accel_out[2] = acceldata.data.reading.z;
    drop_flag = 0;
  }
  else {
    drop_flag = 1;
  }

  // Begin calibration until complete.
  if (!is_calibrated) {
    calibration_vals[0][calibration_count] = accel_out[0];
    calibration_vals[1][calibration_count] = accel_out[1];
    calibration_vals[2][calibration_count] = accel_out[2];
    calibration_count++;
    if (calibration_count == CALIBRATION_LENGTH) {
      for(i = 0; i < CALIBRATION_LENGTH; i++) {
        calibration_acc[0] += calibration_vals[0][i];
        calibration_acc[1] += calibration_vals[1][i];
        calibration_acc[2] += calibration_vals[2][i];
      }
      offset[0] = calibration_acc[0] / CALIBRATION_LENGTH;
      offset[1] = calibration_acc[1] / CALIBRATION_LENGTH;
      offset[2] = calibration_acc[2] / CALIBRATION_LENGTH;
      is_calibrated = true;
    }
  }

  // Run Kalman filter.
  if (is_calibrated) {
    // Bring the wheel encoder outputs into the dr domain
    for(i = 0; i < 4; i++) {
      encoder_speeds[i] = (float)encoder_speed(i)*QUARTERDEGREE_TO_MS;
    }
    speed4_to_speed3(encoder_speeds, wheel_speeds);
    rotate(wheel_speeds, current_state.angle);
    kalman_data.wheels_x = wheel_speeds[0];
    kalman_data.wheels_y = wheel_speeds[1];
    kalman_data.wheels_t = wheel_speeds[2];

    // Bring the accelerometer outputs into the dr domain.
    robot_accels[0] = -M_S_2_PER_ACCEL*(accel_out[1] - offset[1]);
    robot_accels[1] = M_S_2_PER_ACCEL*(accel_out[0] - offset[0]);
    robot_accels[2] = -M_S_2_PER_ACCEL*(accel_out[2] - offset[2]);
    rotate(robot_accels, current_state.angle);
    kalman_data.accelerometer_x = robot_accels[0];
    kalman_data.accelerometer_y = robot_accels[1];
    kalman_data.accelerometer_z = robot_accels[2];

    // Bring the gyro output into the dr domain.
    kalman_data.gyro = gyro_speed/ROBOT_RADIUS;

    kalman_step(&current_state, &kalman_data);
  }
  else {
    current_state.x = 0;
    current_state.y = 0;
    current_state.angle = 0;
    current_state.vx = 0;
    current_state.vy = 0;
    current_state.avel = 0;
  }

  // Coordinates must be transformed to correct for accelerometer orientation
  // on board.

  //robot_accels[0] = -M_S_2_PER_ACCEL*(accel_out[1] - offset[1]);
  //robot_accels[1] = M_S_2_PER_ACCEL*(accel_out[0] - offset[0]);
  //robot_accels[2] = -M_S_2_PER_ACCEL*(accel_out[2] - offset[2]);
  //rotate(robot_accels, current_state.angle);
  //

  //
  //
  //current_state.x += 0.5f*robot_accels[0]*TICK_TIME*TICK_TIME + current_state.vx*TICK_TIME;
  //current_state.y += 0.5f*robot_accels[1]*TICK_TIME*TICK_TIME + current_state.vy*TICK_TIME;
  //current_state.vx += robot_accels[0]*TICK_TIME;
  //current_state.vy += robot_accels[1]*TICK_TIME;
  //drop_flag = 0;


	//current_state.avel = robot_speeds[2]/ROBOT_RADIUS;
	//current_state.angle += current_state.avel*TICK_TIME;

	if (log) {
		log->tick.dr_x = current_state.x;
		log->tick.dr_y = current_state.y;
		log->tick.dr_angle = current_state.angle;
		log->tick.dr_vx = current_state.vx;
		log->tick.dr_vy = current_state.vy;
		log->tick.dr_avel = current_state.avel;
	}

  //printf("%i\t%i\t%f\n", is_calibrated, gyrodata.status, current_state.avel);
}

/**
 * \brief provides current state information to caller
 */
void dr_get(dr_data_t *ret) {
	*ret = current_state;
}

/**
 * \brief provides current Kalman (i.e. sensor) information to caller.
 */
void kalman_get(kalman_data_t *ret) {
  *ret = kalman_data;
}



/**
 * \brief sets the applied accels for use by the kalman filter
 */
void dr_setaccel(float linear_accel[2], float angular_accel) {

  float dr_linear_accel[2];
  dr_linear_accel[0] = linear_accel[0];
  dr_linear_accel[1] = linear_accel[1];
  // Rotate accels from robot coordinates to dr coordinates.
  rotate(dr_linear_accel, current_state.angle);
  kalman_data.x_accel = dr_linear_accel[0];
  kalman_data.y_accel = dr_linear_accel[1];
  kalman_data.t_accel = angular_accel;
}


/**
 * \brief Sets the robot's camera frame.
 */
void dr_set_robot_frame(int16_t x, int16_t y, int16_t angle) {

  robot_camera_data.x = x;
  robot_camera_data.y = y;
  robot_camera_data.angle = angle;

  kalman_data.cam_x = (float)(x/1000.0);
  kalman_data.cam_y = (float)(y/1000.0);
  kalman_data.cam_t = (float)(angle/1000.0);
  kalman_data.new_camera_data = true;
}

/**
 * \brief Sets the ball's camera frame.
 */
void dr_set_ball_frame(int16_t x, int16_t y) {
  ball_camera_data.x = x;
  ball_camera_data.y = y;
}

/**
 * \brief Sets the robot's camera frame timestamp.
 */
void dr_set_robot_timestamp(uint64_t timestamp) {
  robot_camera_data.timestamp = timestamp;
}

/**
 * \brief Sets the ball's camera frame timestamp.
 */
void dr_set_ball_timestamp(uint64_t timestamp) {
   ball_camera_data.timestamp = timestamp;
}



