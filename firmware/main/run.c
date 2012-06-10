#include "mrf.h"
#include "mac.h"
#include "io.h"
#include "run.h"
#include "led.h"
#include "control.h"
#include "encoder.h"
#include "motor.h"

int16_t wheel_setpoint[4];

//called once at startup
void startup() {	
}


//called at maximum possible rate (while waiting for tick)
void fast_poll() {
}


//called once every 5 ms
void tick() {
	set_test_leds(USER_MODE,7);
	for(uint8_t i=0;i<4;i++) {
		int16_t enc_val = read_encoder(i);
		int16_t output = control_iteration(controller_state[i],wheel_setpoint[i], enc_val);
		if(output > 0) {
				set_wheel(i,FORWARD,output);
			} else { 
				if(output < 0) {
					set_wheel(i,BACKWARD,-1*output);
				} else {
					set_wheel(i,BRAKE,0);
				}
		}
	}
	set_test_leds(USER_MODE,0);
}
