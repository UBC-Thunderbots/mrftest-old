#include "control.h"
#include "wheels.h"
#include <string.h>
#include <math.h>

//All wheels good
static const float speed4_to_speed3_mat[5][3][4]=
{
	{ 
		{-0.34847, -0.29380,0.29380,0.34847},
    {0.39944,-0.39944,-0.39944, 0.39944},
    {0.28245, 0.21755, 0.21755, 0.28245}
	},
	{
		{0, -0.70710,  0.70710, 0},
		{0,  0.07432, -0.87320, 0.79888},
		{0,  0.55255, -0.11745, 0.05649}
	},
	{ 
		{-0.59618, 0, 0, 0.59618},
		{ 0.06266, 0, -0.7988, 0.073622},
		{ 0.46587, 0, 0.43510, 0.099024}
	},
	{
		{-0.59618, 0, 0, 0.59618},
    {0.73622, -0.79888, 0, 0.062659},
    {0.099024, 0.43510, 0, 0.46587}
	},
	{
		{0, -0.70710, 0.70710, 0},
    {0.7988, -0.87320, 0.074317, 0},
		{0.5649, -0.11745, 0.55255, 0}
	}
};

static const  float slip_vector[4] = {-0.45580, 0.54060, -0.54060, 0.45580};

static const float speed3_to_speed4_mat[4][3] =
	{ 
		{-0.83867, 0.54464, 1.00000},
    { -0.70711,-0.70711, 1.00000},
    {0.70711,-0.70711, 1.00000},
    {0.83867, 0.54464, 1.00000}
	};

#define QUARTERDEGREE_TO_MS 0.0114f
#define ROBOT_RADIUS 0.08f
#define TICK_TIME 0.005f
#define DELTA_VOLTAGE_LIMIT 4.706f  //Voltage where wheel slips
#define VOLTAGE_FROM_ENCODER 0.0223f
#define VOLTAGE 16.0f
#define AGGRESSIVENESS 1.0f
#define AGGRESSIVENESS_MAX 10.0f
#define INPUT_NOISE 0.02f //noise of speed in m/s (measured value)

//convert quarter degree 4 speeds to m/s 3 speed
static void speed4_to_speed3(const int16_t speed4[4], float speed3[3]) {
	for(uint8_t j=0;j<3;++j) {
		speed3[j]=0;
		for(uint8_t i=0;i<4;++i) {
			speed3[j]+=QUARTERDEGREE_TO_MS*speed4_to_speed3_mat[0][j][i]*speed4[i];
		}
	}
}

//convert m/s 3speed to quarter degree 4 speed
static void speed3_to_speed4(const float speed3[3], float speed4[4]) {
	for(uint8_t j=0;j<4;++j) {
		speed4[j]=0;
		for(uint8_t i=0;i<3;++i) {
			speed4[j] += (1.0f/QUARTERDEGREE_TO_MS) * speed3_to_speed4_mat[j][i]*speed3[i]; 
		}
	}
}

static void rotate_velocity(float speed[3], float angle) {
	float temp = cos(angle)*speed[0] - sin(angle)*speed[1];
	speed[1] = sin(angle)*speed[0] + cos(angle)*speed[1];
	speed[0]=temp;
}

void control_clear() {
	//current controller has no persistent other than setpoint state
}

void control_process_new_setpoints(const int16_t setpoints[4]) {
	speed4_to_speed3(setpoints, wheels_setpoints.robot);
}

void control_tick(float battery) {
	float Velocity[3];
	float Veldiff[3];
	float Accels[4];
	float max_accel=-10;
	float min_rescale_factor=1;
	
	//Convert the measurements to the 3 velocity
	speed4_to_speed3(wheels_encoder_counts, Velocity);
	//This needs to be changed to the state updater

	//Update the setpoint by the current rotational speed
	rotate_velocity(wheels_setpoints.robot,-Velocity[2]/ROBOT_RADIUS*TICK_TIME);

	//Get the control error
	for(uint8_t i=0;i<3;++i) {
		Veldiff[i]=wheels_setpoints.robot[i]-Velocity[i];
	}

	//convert that control error in the 4 wheel accel direction
	speed3_to_speed4(Veldiff,Accels);
	
	for(uint8_t i=0;i<4;++i) {
		//accel from vel related by motor constant
		Accels[i] = AGGRESSIVENESS*(DELTA_VOLTAGE_LIMIT/AGGRESSIVENESS_MAX)*Accels[i]/(INPUT_NOISE / QUARTERDEGREE_TO_MS); 		
		if(fabsf(Accels[i]) > max_accel) {
			max_accel = fabsf(Accels[i]);
		}
	}

	if(max_accel > DELTA_VOLTAGE_LIMIT) {
		for(uint8_t i=0;i<4;++i) {
			Accels[i] = (DELTA_VOLTAGE_LIMIT/max_accel)*Accels[i];
		}
	}
	
	for(uint8_t i=0;i<4;++i) {
		if(fabsf((VOLTAGE - wheels_encoder_counts[i]*VOLTAGE_FROM_ENCODER)/Accels[i]) < min_rescale_factor) {
			min_rescale_factor = fabsf((VOLTAGE - wheels_encoder_counts[i]*VOLTAGE_FROM_ENCODER)/Accels[i]);
		}
	}

	for(uint8_t i=0;i<4;++i) {
		float	temp = (wheels_encoder_counts[i]*VOLTAGE_FROM_ENCODER+ min_rescale_factor*Accels[i])/VOLTAGE*255;
		if(temp > 255) {
			wheels_drives[i] = 255;
		} else {
			if(temp < -255) {
				wheels_drives[i] = -255;
			} else {
				wheels_drives[i] = temp;
			}
		}
	}
}
