#ifndef DR_H
#define DR_H

#include "log.h"

// 1 second worth of samples.
#define CALIBRATION_LENGTH 200

/**
 * \brief The type of data returned by the dead reckoning module.
 *
 * The following units are used:
 * \li Linear positions are in metres.
 * \li Orientations are in radians.
 * \li Linear velocities are in metres per second.
 * \li Angular velocities are in radians per second.
 *
 * The following coordinate system is used:
 * \li Positive X is in the direction the robot was facing at the last call to
 * \ref dr_reset.
 * \li Positive Y is 90° to the left of positive Y.
 * \li Positive angles are leftward rotation.
 *
 * The origin is the position and orientation of the robot at the last call to
 * \ref dr_reset.
 */
typedef struct {
	/**
	 * \brief The X component of the robot’s accumulated motion.
	 */
	float x;

	/**
	 * \brief The Y component of the robot’s accumulated motion.
	 */
	float y;

	/**
	 * \brief The angular component of the robot’s accumulated motion.
	 */
	float angle;

	/**
	 * \brief The X component of the robot’s velocity.
	 */
	float vx;

	/**
	 * \brief The Y component of the robot’s velocity.
	 */
	float vy;

	/**
	 * \brief The robot’s angular velocity.
	 */
	float avel;
} dr_data_t;

/**
 *  \brief The required sensor and acceleration data for the kalman filter.
 */
typedef struct {
  float cam_x;
  float cam_y;
  float cam_t;
  float x_accel;
  float y_accel;
  float t_accel;
  float accelerometer_x;
  float accelerometer_y;
  float accelerometer_z;
  float gyro;
  float wheels_x;
  float wheels_y;
  float wheels_t;
  bool new_camera_data;
} kalman_data_t;



typedef struct {
  /**
  * \brief The x component of the robot's global position in mm.
  */
  int16_t x;

  /**
  * \brief The y component of the robot's global position in mm.
  */
  int16_t y;

  /**
  * \brief The theta component of the robot's pose in the global frame in mrad.
  */
  int16_t angle;
  
  /**
  * \brief The timestamp associated with this camera frame.
  */
  uint64_t timestamp;
  
} robot_camera_data_t;

typedef struct {
  /**
  * \brief The x component of the ball's global position in mm.
  */
  int16_t x;

  /**
  * \brief The y component of the ball's global position in mm.
  */
  int16_t y;

  /**
  * \brief The timestamp associated with this camera frame.
  */
  uint64_t timestamp;

} ball_camera_data_t;


void dr_init(void);
bool dr_calibrated(void);
void dr_reset(void);
void dr_tick(log_record_t *log);
void dr_get(dr_data_t *ret);
void kalman_get(kalman_data_t *ret);
void dr_setaccel(float linear_accel[2], float angular_accel);
void dr_set_robot_frame(int16_t x, int16_t y, int16_t angle);
void dr_set_ball_frame(int16_t x, int16_t y);
void dr_set_robot_timestamp(uint64_t timestamp);
void dr_set_ball_timestamp(uint64_t timestamp);

#endif
