#include "control.h"
#include "adc.h"
#include "wheels.h"
#include <string.h>
#include <math.h>

#define QUARTERDEGREE_TO_MS 0.0114f
#define ROBOT_RADIUS 0.08f
#define TICK_TIME (1.0f / CONTROL_LOOP_HZ)
#define DELTA_VOLTAGE_LIMIT 8.0f  //Voltage where wheel slips
#define ROBOT_MASS 2.48f
#define INERTIAL_CONSTANT 0.282f
#define AGGRESSIVENESS 0.08f
#define INPUT_NOISE 0.02f //noise of speed in m/s (measured value)

//All wheels good
static const float speed4_to_speed3_mat[3][4]=
{
	{-0.34847, -0.29380,0.29380,0.34847},
	{0.39944,-0.39944,-0.39944, 0.39944},
	{0.28245, 0.21755, 0.21755, 0.28245}
};

static const float speed3_to_speed4_mat[4][3] =
	{ 
		{-0.83867, 0.54464, 1.00000},
    { -0.70711,-0.70711, 1.00000},
    {0.70711,-0.70711, 1.00000},
    {0.83867, 0.54464, 1.00000}
	};

static const float rescale_vector[] = {AGGRESSIVENESS, AGGRESSIVENESS, AGGRESSIVENESS };
static const float slip_vector[4] = {-0.45580, 0.54060, -0.54060, 0.45580};
#if 0
static const float filter_gain = 0.0362;
static const float filter_B[] = {1, 1.2857, 1.2857, 1};
static const float filter_A[] = {1.0, -1.7137, 1.1425, -0.2639};
#define FILTER_ORDER 3
static float input_filter_states[4][FILTER_ORDER];
#else
static const float filter_gain = 0.0246;
static const float filter_B[] = {1, 0.66667, 1};
static const float filter_A[] = {1.0, -1.6079, 0.6735};
#define FILTER_ORDER 2
static float input_filter_states[4][FILTER_ORDER];
#endif
static float setpoints[3U];

static float runDF2(float input,float gain,const float* num,const float* den, float* state, size_t order) {
	float accum=0;
	for(size_t i=0;i<order;++i) {
		input -= state[i]*den[i+1];
	}
	input /= den[0];

	for(size_t i=order-1;i!=0;--i) {
		accum += state[i]*num[i+1];
		state[i]=state[i-1];
	}
		state[0]=input;
		accum += input*num[0];
	return gain*accum;
}


//convert 4 speeds to 3 speed
static void speed4_to_speed3(const float speed4[4], float speed3[3]) {
	for(unsigned int j=0;j<3;++j) {
		speed3[j]=0;
		for(unsigned int i=0;i<4;++i) {
			speed3[j]+= speed4_to_speed3_mat[j][i]*speed4[i];
		}
	}
}

//convert 3 speed to 4 speed
static void speed3_to_speed4(const float speed3[3], float speed4[4]) {
	for(uint8_t j=0;j<4;++j) {
		speed4[j]=0;
		for(unsigned int i=0;i<3;++i) {
			speed4[j] +=  speed3_to_speed4_mat[j][i]*speed3[i]; 
		}
	}
}

static void rotate_velocity(float speed[3], float angle) {
	float temp = cosf(angle)*speed[0] - sinf(angle)*speed[1];
	speed[1] = sinf(angle)*speed[0] + cosf(angle)*speed[1];
	speed[0]=temp;
}

void control_clear() {
	//current controller has no persistent other than setpoint state
}

void control_process_new_setpoints(const int16_t wheel_setpoints[4]) {
	float temp[4];
	for(unsigned int i=0;i<4;++i) {
		temp[i]= QUARTERDEGREE_TO_MS*wheel_setpoints[i];
	}
	speed4_to_speed3(temp, setpoints);
}

void control_tick(const int16_t feedback[4U], int16_t drive[4U]) {
	float Velocity[3];
	float Veldiff[3];
	float Accels[4];
	float max_accel=-10;
	float min_rescale_factor=1;
	float filtered_encoders[4];
	for(size_t i=0;i<4;++i) {
		filtered_encoders[i] = runDF2(feedback[i],filter_gain,filter_B,filter_A,input_filter_states[i],FILTER_ORDER);
	}

	//Convert the measurements to the 3 velocity
	speed4_to_speed3(filtered_encoders, Velocity);
	//This needs to be changed to the state updater

	//Update the setpoint by the current rotational speed
	rotate_velocity(setpoints,-Velocity[2]/ROBOT_RADIUS*QUARTERDEGREE_TO_MS*TICK_TIME);

	//Get the control error
	for(uint8_t i=0;i<3;++i) {
		Veldiff[i]=(setpoints[i]-Velocity[i]*QUARTERDEGREE_TO_MS)*(DELTA_VOLTAGE_LIMIT/INPUT_NOISE)*rescale_vector[i]; //Should be desired Acceleration
	}

	//convert that control error in the 4 wheel accel direction
	speed3_to_speed4(Veldiff,Accels);
	
	for(uint8_t i=0;i<4;++i) {
		if(fabsf(Accels[i]) > max_accel) {
			max_accel = fabsf(Accels[i]);
		}
	}

	if(max_accel > DELTA_VOLTAGE_LIMIT) {
		for(uint8_t i=0;i<4;++i) {
			Accels[i] = (DELTA_VOLTAGE_LIMIT/max_accel)*Accels[i];
		}
	}
	
	float battery = adc_battery();
	for(uint8_t i=0;i<4;++i) {
		if(fabsf((battery - feedback[i]*WHEELS_VOLTS_PER_ENCODER_COUNT)/Accels[i]) < min_rescale_factor) {
			min_rescale_factor = fabsf((battery - feedback[i]*WHEELS_VOLTS_PER_ENCODER_COUNT)/Accels[i]);
		}
	}

	for(uint8_t i=0;i<4;++i) {
		float	temp = (feedback[i]*WHEELS_VOLTS_PER_ENCODER_COUNT+ min_rescale_factor*Accels[i])/battery*255;
		//because I can
		drive[i] = (temp > 255)?255:(temp<-255)?-255:temp;
	}
}
