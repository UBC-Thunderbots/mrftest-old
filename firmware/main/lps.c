#include "lps.h"
#include "adc.h"
#include "pins.h"
#include <gpio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "tbuf.h"
//LPS0 = PC0
//LPS1 = PC1
//LPS2 = PB4
//LPS3 = PB5

#define LPS_SENSOR_SPACING 2 // cm
//#define LPS_OFFSET 0.03f
#define LPS_MIN 0.001f
#define LPS_THRESH -0.15f
#define LPS_ARRAY_SIZE 4

typedef float lps_values[LPS_ARRAY_SIZE];
typedef float lps_adc[16];

static lps_values lps_raw;
static lps_values lps_norm;
static float lps_mean;
static float lps_var;

// buffer to be accessed by isr and normal task
static unsigned int current_lps_buffer;
static lps_values lps_buf[3U];

static tbuf_t lps_buffer_ctl = TBUF_INIT;

void lps_init(void){
	current_lps_buffer = tbuf_write_get(&lps_buffer_ctl);
}

// math implemented
/* (4x1)      (4x16)               (16x4)     
 * [LSB     [...(-1)^(n+1).....    [16x1
 *   .   =   ...(-1)^(n/2+1)...     ADC
 *   .       ...(-1)^(n/4+1)...  *  Values] 
 *  MSB]     ...(-1)^(n/8+1)...]
 *
 */

//static lps_values* updating_lps; 
//static lps_values* ready_lps_val;
//static lps_adc adc_values = {0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0};

void lps_incr(void){
	static unsigned int counter = 0;
	//static lps_values updating_lps = {0,0,0,0};	
	float adc_reading = adc_lps();
	float* updating_lps;
	
	//adc_values[counter%16]=adc_reading;
	
	updating_lps = lps_buf[current_lps_buffer];
	for(unsigned int i = 0; i<LPS_ARRAY_SIZE; i++)
	{
		updating_lps[i]+=((counter&(1<<i))?1:-1)*adc_reading;
	}

	counter++; 
	counter=counter%16;
		
	if((counter>>0)&1) {gpio_set(PIN_LPS_DRIVE3);} else {gpio_reset(PIN_LPS_DRIVE3);}
	if((counter>>1)&1) {gpio_set(PIN_LPS_DRIVE2);} else {gpio_reset(PIN_LPS_DRIVE2);}
	if((counter>>2)&1) {gpio_set(PIN_LPS_DRIVE1);} else {gpio_reset(PIN_LPS_DRIVE1);}
	if((counter>>3)&1) {gpio_set(PIN_LPS_DRIVE0);} else {gpio_reset(PIN_LPS_DRIVE0);}

	
	if( counter == 0 ){
		tbuf_write_put(&lps_buffer_ctl, current_lps_buffer);
		current_lps_buffer = tbuf_write_get(&lps_buffer_ctl);
		updating_lps = lps_buf[current_lps_buffer];
		for( unsigned int i = 0; i < LPS_ARRAY_SIZE; i++ ){
			updating_lps[i] = 0.0;
		}
	}
}

void lps_tick (void){
	// when the calculation is complete 
	float* updating_lps;
	unsigned int index;
	float lps_sum = 0.0;

	index = tbuf_read_get(&lps_buffer_ctl);
	updating_lps = lps_buf[index];

	for(unsigned int i = 0; i<LPS_ARRAY_SIZE; i++){
		if(updating_lps[i] < LPS_THRESH){
			lps_raw[i] = -updating_lps[i];
		} else {
			lps_raw[i] = LPS_MIN;
		}
		updating_lps[i]=0;
	}
	lps_mean = 0.0;
	lps_var = 0.0;
	for(unsigned int i = 0; i<LPS_ARRAY_SIZE; i++){
		lps_sum += lps_raw[i];
	}
	for(unsigned int i = 0; i<LPS_ARRAY_SIZE; i++){
		lps_norm[i] = lps_raw[i]/lps_sum;
		lps_mean += lps_norm[i] * (i-1.5f) * LPS_SENSOR_SPACING;	
		lps_var += lps_norm[i] * (i-1.5f)*(i-1.5f) * LPS_SENSOR_SPACING * LPS_SENSOR_SPACING;
	}
	lps_var = lps_var - lps_mean*lps_mean;
	tbuf_read_put(&lps_buffer_ctl, index);

}

void lps_print(void){
	unsigned int i=0;
	unsigned int j=0;
	/*printf("adc[");
	for(j=0; j< 4; j++){
		for(i=0; i< 4; i++){
			printf("%f ", adc_values[j*4+i]);
		}
		printf("\r\n");
	}
	printf("]\r\n");*/
	printf("lps_raw(%f, %f, %f, %f) \r\n", lps_raw[0], lps_raw[1], lps_raw[2], lps_raw[3]);
	printf("lps_norm(%f, %f, %f, %f) mean=%f, var=%f\r\n", lps_norm[0], lps_norm[1], lps_norm[2], lps_norm[3], lps_mean, lps_var);

}


