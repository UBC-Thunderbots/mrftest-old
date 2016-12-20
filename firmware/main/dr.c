#include "dr.h"
#include "dsp.h"
#include "encoder.h"
#include "physics.h"
#include "sensors.h"

#if 0
static const float filter_gain = 0.0362f;
static const float filter_B[] = {1.0f, 1.2857f, 1.2857f, 1.0f};
static const float filter_A[] = {1.0f, -1.7137f, 1.1425f, -0.2639f};
#define FILTER_ORDER 3
static float input_filter_states[4][FILTER_ORDER];
#else
static const float filter_gain = 0.0246f;
static const float filter_B[] = {1.0f, 0.66667f, 1.0f};
static const float filter_A[] = {1.0f, -1.6079f, 0.6735f};
#define FILTER_ORDER 2
static float encoder_filter_states[4][FILTER_ORDER];
static float hall_filter_states[4][FILTER_ORDER];
#endif


static float runEncoderFilter(float input,float* filter_states) {
	return filter_gain*runDF2(input,filter_B,filter_A,filter_states,FILTER_ORDER);
}

static dr_data_t current_state;

/**
 * \brief The most recent ball and camera data frames.
 */
static robot_camera_data_t robot_camera_data;
static ball_camera_data_t ball_camera_data;

/**
 * \brief called a system boot to configure deadreckoning system
 */
void dr_init(void) {
  robot_camera_data.x = 0;
  robot_camera_data.y = 0;
  robot_camera_data.angle = 0;
  robot_camera_data.timestamp = 0;
  
  ball_camera_data.x = 0;
  ball_camera_data.y = 0;
  ball_camera_data.timestamp = 0;  
}

/**
 * \brief resets the position to origin but preserves additional state
 */
void dr_reset(void) {
	current_state.x = 0.0f;
	current_state.y = 0.0f;
	current_state.angle = 0.0f;
}

/**
 * \brief called on tick after encoders and gyros are read to update state
 *
 * \param[out] log the log record to fill, if any
 */
void dr_tick(log_record_t *log) {
	//with proper tracking this likely does not need to be filtered
	float encoder_speeds[4];
	//float hall_speeds[4];
	
	for(int i=0;i<4;++i) {
		//encoder_speeds[i] = runEncoderFilter(((float)encoder_speed(i))*QUARTERDEGREE_TO_MS, encoder_filter_states[i]);
		encoder_speeds[i] = (float)encoder_speed(i)*QUARTERDEGREE_TO_MS;
		//hall_speeds[i] = runEncoderFilter((float)hall_speed(i)*WHEELS_HALL_TO_MS, hall_filter_states[i]);
	}
	
	//ignore gyros and accelerometer for now
	//sensors_gyro_data_t gyrodata = sensors_get_gyro();	
	//if(gyrodata.status) {
		 // MS_PER_GYRO*gyrodata.data.reading.z;
	//}

	float robot_speeds[3];

	speed4_to_speed3(encoder_speeds, robot_speeds);
	rotate(robot_speeds, current_state.angle);
	current_state.avel = robot_speeds[2]/ROBOT_RADIUS;
	current_state.vx = robot_speeds[0];
	current_state.vy = robot_speeds[1];
	//Temporarily trusting camera data for current state
	//Adding in time adjustment and kalman code in future
	//current_state.x += current_state.vx*TICK_TIME;
	//current_state.y += current_state.vy*TICK_TIME;
	//current_state.angle += current_state.avel*TICK_TIME;

	current_state.x = robot_camera_data.x;
	current_state.y = robot_camera_data.y;
	current_state.angle = robot_camera_data.angle;

	if (log) {
		log->tick.dr_x = current_state.x;
		log->tick.dr_y = current_state.y;
		log->tick.dr_angle = current_state.angle;
		log->tick.dr_vx = current_state.vx;
		log->tick.dr_vy = current_state.vy;
		log->tick.dr_avel = current_state.avel;
	}
}

/**
 * \brief provides current state information to caller
 */
void dr_get(dr_data_t *ret) {
	*ret = current_state;
}

/**
 * \brief Sets the ball's camera frame.
 */
void dr_set_ball_frame(int16_t x, int16_t y) {
  ball_camera_data.x = x;
  ball_camera_data.y = y;
}

/**
 * \brief Sets the robot's camera frame.
 */
void dr_set_robot_frame(int16_t x, int16_t y, int16_t angle) {
  robot_camera_data.x = x;
  robot_camera_data.y = y;
  robot_camera_data.angle = angle;
}

/**
 * \brief Sets the ball's camera frame timestamp.
 */
void dr_set_ball_timestamp(uint64_t timestamp) {
   ball_camera_data.timestamp = timestamp;
}

/**
 * \brief Sets the robot's camera frame timestamp.
 */
void dr_set_robot_timestamp(uint64_t timestamp) {
  robot_camera_data.timestamp = timestamp;
}
