#include "lps.h"
#include "adc.h"
#include "pins.h"
#include <gpio.h>
#include <stddef.h>
#include <stdint.h>

//LPS0 = PC0
//LPS1 = PC1
//LPS2 = PB4
//LPS3 = PB5

lps_values lps_export;

void lps_init(){
	// nothing to initiate so far
	
}

// math implemented
/* (4x1)      (4x16)               (16x4)     
 * [LSB     [...(-1)^(n+1).....    [16x1
 *   .   =   ...(-1)^(n/2+1)...     ADC
 *   .       ...(-1)^(n/4+1)...  *  Values] 
 *  MSB]     ...(-1)^(n/8+1)...]
 *
 */


void lps_incr(){
	static unsigned int counter = 0;
	static lps_values updating_lps = {0,0,0,0};	
	float adc_reading = adc_lps();
	
	for(unsigned int i = 0; i<LPS_ARRAY_SIZE; i++){
		updating_lps[i]=((counter&(1<<i))?1:-1)*adc_reading;
	}
		
	if((counter>>0)&1) {gpio_set(PIN_LPS_DRIVE0);} else {gpio_reset(PIN_LPS_DRIVE0);}
	if((counter>>1)&1) {gpio_set(PIN_LPS_DRIVE1);} else {gpio_reset(PIN_LPS_DRIVE1);}
	if((counter>>2)&1) {gpio_set(PIN_LPS_DRIVE2);} else {gpio_reset(PIN_LPS_DRIVE2);}
	if((counter>>3)&1) {gpio_set(PIN_LPS_DRIVE3);} else {gpio_reset(PIN_LPS_DRIVE3);}

	if( (counter%(1<<LPS_ARRAY_SIZE) ) ){
		for(unsigned int i = 0; i<LPS_ARRAY_SIZE; i++){
			lps_export[i] = updating_lps[i];
		}
	}

	counter++; //letting the number naturally roll over
}

int lps_get(){
	

	return 0;
}


