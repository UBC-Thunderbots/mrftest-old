#include <stdio.h>
#include <stdlib.h>
#include "simulate.h"
//#include "spline.h"
#include "shoot.h"
// #include "move.h"
#include "primitive.h"
#include <stdint.h>
#include <stdlib.h>
#include "physics.h"


#define DELTA_T 0.0001
#define ROBOT_TICK_T 0.005
#define MAX_SIM_T 15.0
#define HIST_TICK_T 0.03
#define NUM_PARAMS 3
#define NUM_ATTEMPTS 1

static const unsigned HIST_SIZE = MAX_SIM_T/HIST_TICK_T + 1;
const float X_BALL = 1.0;
const float Y_BALL = 2.0;

double metric(dr_data_t hist[HIST_SIZE], unsigned histPos){
	float cost;
	for(unsigned i=0;i++;i<histPos){
		cost += hist[i].x*hist[i].x + hist[i].y*hist[i].y;
	}
	cost = cost/histPos;
	return cost;
}

unsigned runSim(double params[NUM_PARAMS], dr_data_t hist[HIST_SIZE]){
	sim_reset();
	primitive_params_t p;
	p.params[0] = (int16_t)(X_BALL * 1000); // final x position
	p.params[1] = (int16_t)(Y_BALL * 1000); // final y position 
	p.params[2] = (int16_t)(0.0 * 100);  // final rotation angle
	p.params[3] = (int16_t)(0.0 * 1000);
	shoot_start(&p);
	//move_start(&p);
	sim_log_start();	

	float time = 0.0;
	float last_robot_tick = 0.0;
	float last_log_tick = 0.0;
	float last_hist_tick = 0.0;
	unsigned histPos = 0;
	float x;
	while(time < MAX_SIM_T){
		time += DELTA_T;
		sim_tick(DELTA_T);
		
		if(time - last_robot_tick >= ROBOT_TICK_T){
			shoot_tick();
			// move_tick();
			last_robot_tick = time;
		}

		if(time - last_log_tick >= LOG_TICK_T){
			sim_log_tick(time);
			last_log_tick = time;
		}
		if(time - last_hist_tick >= HIST_TICK_T){
			dr_get(&(hist[histPos]));	
			histPos++;	
			last_hist_tick = time;
		}

		x = get_pos_x();
		float end = abs((int) (p.params[0] / 1000));
		if (abs(x * 1000) / 1000.0f >= end) {
			break;
		}


	}
	return histPos;
}

double optimise(){
	dr_data_t hist[HIST_SIZE];	
	double params[NUM_PARAMS];
	unsigned histPos;
	double quality;
		
	for(unsigned i=0;i<NUM_ATTEMPTS;i++){
		params[0] = 1.0;
		params[1] = 5.0;
		params[2] = 0.0;
	
		histPos = runSim(params, hist);

		quality = metric(hist, histPos);
		//printf("%f\n",quality);		
	}
}

int main(int argc, char **argv)
{
	optimise();
	return 0;
}
