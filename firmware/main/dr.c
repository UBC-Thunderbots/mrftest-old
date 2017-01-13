#include "dr.h"
#include "dsp.h"
#include "encoder.h"
#include "physics.h"
#include "sensors.h"
#include "circbuff.h"
#include "primitives/primitive.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>

static dr_data_t current_state;

/**
 * \brief The most recent ball and camera data frames.
 */
static robot_camera_data_t robot_camera_data;
static ball_camera_data_t ball_camera_data;
static wheel_speeds_t past_wheel_speeds[SPEED_SIZE];

//Variables for hard coded drive pattern (testing) 
static uint16_t tick_count = 0; 
static int maneuver_stage = 0;

/**
 * \brief called a system boot to configure deadreckoning system
 */
void dr_init(void) {
  current_state.x = 0.0f;
  current_state.y = 0.0f;
  current_state.angle = 0.0f;

  ball_camera_data.x = 0.0;
  ball_camera_data.y = 0.0;
  ball_camera_data.timestamp = 0;

  circbuff_init(past_wheel_speeds, SPEED_SIZE);
}


/**
 * \brief called on tick after accel and gyros are read to update state
 *
 * \param[out] log the log record to fill, if any
 */
void dr_tick(log_record_t *log) {
  tick_count++;

  //New camera data- update dead reckoning
  if(robot_camera_data.new_data){
    dr_apply_cam();
  }

  float encoder_speeds[4];
  float wheel_speeds[3];

  for(unsigned int i = 0; i < 4; i++) {
      encoder_speeds[i] = (float)encoder_speed(i)*QUARTERDEGREE_TO_MS;
  }
  speed4_to_speed3(encoder_speeds, wheel_speeds);
  wheel_speeds_t wheel_speed_object;
  wheel_speed_object.speed_x = wheel_speeds[0];
  wheel_speed_object.speed_y = wheel_speeds[1];
  wheel_speed_object.speed_angle = wheel_speeds[2]/ROBOT_RADIUS;
  addToQueue(past_wheel_speeds, SPEED_SIZE, wheel_speed_object);

  rotate(wheel_speeds, current_state.angle);

  current_state.x += wheel_speed.speed_x*TICK_TIME;
  current_state.y += wheel_speed.speed_y*TICK_TIME;
  current_state.angle += wheel_speed.speed_angle*TICK_TIME;
  current_state.vx = wheel_speed.speed_x;
  current_state.vy = wheel_speed.speed_y;
  current_state.avel = wheel_speed.speed_angle;

  if(current_state.angle > M_PI) current_state.angle -= 2*M_PI;
  else if(current_state.angle < -M_PI) current_state.angle += 2*M_PI;  

  if (log) {
    dr_log(log);
  }
}

/**
 * \brief provides current state information to caller
 */
void dr_get(dr_data_t *ret) {
	*ret = current_state;
}

/**
 * \brief sets the applied accels
 */
void dr_setaccel(float linear_accel[2], float angular_accel) {
  float dr_linear_accel[2];
  dr_linear_accel[0] = linear_accel[0];
  dr_linear_accel[1] = linear_accel[1];
}


/**
 * \brief Sets the robot's camera frame.
 */
void dr_set_robot_frame(int16_t x, int16_t y, int16_t angle) {
  robot_camera_data.x = (float)(x/1000.0);
  robot_camera_data.y = (float)(y/1000.0);
  robot_camera_data.angle = (float)(angle/1000.0);
  robot_camera_data.new_data = true;
}


void dr_apply_cam() {
  float x = robot_camera_data.x;
  float y = robot_camera_data.y;
  float angle = robot_camera_data.angle;
  
  speed_t wheel_speed;
    
  float wheel_speeds[3];

  int additional_delay = (int)(robot_camera_data.timestamp)/((int)(1000*TICK_TIME)); //In number of robot ticks
  //Todo: make sure delay is less than size of circ buffer
  int total_delay = BASE_CAMERA_DELAY + additional_delay;
  for(int i = total_delay; i >= 0; i--){
    wheel_speed = getFromQueue(speed, SPEED_SIZE, i);

    wheel_speeds[0] = wheel_speed.speed_x;
    wheel_speeds[1] = wheel_speed.speed_y;
    wheel_speeds[2] = wheel_speed.speed_angle;
    
    rotate(wheel_speeds, angle);
    x += wheel_speeds[0]*TICK_TIME;
    y += wheel_speeds[1]*TICK_TIME;
    angle += wheel_speeds[2]*TICK_TIME;
  }
  
  angle = fmod(angle, 2*M_PI);
  if(angle > M_PI) angle -= 2*M_PI;

  current_state.x = x;
  current_state.y = y;
  current_state.angle = angle;  
}


/**
 * \brief Sets the ball's camera frame.
 */
void dr_set_ball_frame(int16_t x, int16_t y) {
  ball_camera_data.x = (float)(x/1000.0);
  ball_camera_data.y = (float)(y/1000.0);
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


void dr_do_maneuver(){
  int16_t dests[3][3] = {{700,-700,0},
			 {2000,1000,30},
			 {0,0,(int16_t)(M_PI/2.0)}};



 if(tick_count > 800 || (get_primitive_index() != 1)){

    tick_count = 0;
    maneuver_stage++;
    if(maneuver_stage >= 3) {
      maneuver_stage = 0;
    }
    
    primitive_params_t move_params;                                                                                                
    move_params.params[0] = dests[maneuver_stage][0];                                                                                         
    move_params.params[1] = dests[maneuver_stage][1];                                                                                        
    move_params.params[2] = dests[maneuver_stage][2];
    primitive_start(1, &move_params);
  }
}

void dr_follow_ball(){
  if(tick_count > 10 || (get_primitive_index() != 1)){
  tick_count = 0;

    primitive_params_t move_params;                              
    move_params.params[0] = ball_camera_data.x;
    move_params.params[1] = ball_camera_data.y;
    move_params.params[2] = 0;
    primitive_start(1, &move_params);
  }
}


dr_log(){
  sensors_gyro_data_t gyrodata;
  sensors_accel_data_t acceldata;
  float gyro_speed;
  int16_t accel_out[3] = {0};
  float encoder_speeds[4];
  float wheel_speeds[3];

  gyrodata = sensors_get_gyro();
  acceldata = sensors_get_accel();

  for(unsigned int i = 0; i < 4; i++) {
      encoder_speeds[i] = (float)encoder_speed(i)*QUARTERDEGREE_TO_MS;
  }

  log->tick.dr_x = current_state.x;
  log->tick.dr_y = current_state.y;
  log->tick.dr_angle = current_state.angle;
  log->tick.dr_vx = current_state.vx;
  log->tick.dr_vy = current_state.vy;
  log->tick.dr_avel = current_state.avel;

  log->tick.enc_vx = wheel_speeds[0];
  log->tick.enc_vy = wheel_speeds[1];
  log->tick.enc_avel = wheel_speeds[2];
  log->tick.accelerometer_x = acceldata.data.reading.x;
  log->tick.accelerometer_y = acceldata.data.reading.y;
  log->tick.accelerometer_z = acceldata.data.reading.z;

  log->tick.gyro_avel = MS_PER_GYRO*gyrodata.data.reading.z;

  log->tick.cam_x = robot_camera_data.x;
  log->tick.cam_y = robot_camera_data.y;
  log->tick.cam_angle = robot_camera_data.angle;

  log->tick.cam_ball_x = ball_camera_data.x;
  log->tick.cam_ball_y = ball_camera_data.y;
				
  log->tick.cam_delay = (uint16_t)robot_camera_data.timestamp;
}

