#include <pic18f4550.h>
#include "pwm.h"


/* Use ADC input to control duty cycle of PWM */
/* Implementation incomplete */

void main(void) {
	ADCON1 = 0x0F;//Setting all pins as digital

	INTCON2bits.RBPU = 0; // Internal pullups for port B enabled
	TRISBbits.TRISB0 = 1;//RB1 set to input
	TRISDbits.TRISD4 = 0;//Red LED
	TRISDbits.TRISD5 = 0;//Green LED 
	TRISDbits.TRISD6 = 0;//Green LED
	LATD = 0x00;

	InitPWM();

	/*ADCON1bits.PCFG0 = 0;//setting RA0 as analog
	ADCON1bits.VCFG1 = 0;//setting voltage reference
	ADCON1bits.VCFG0 = 0;//setting voltage reference
	ADCON0bits.ADON = 1;//Turning on ADC
	//Now i need to set conversion time and acquisition times.
	ADCON2bits.ADCS2 = 1;//
	ADCON2bits.ADCS1 = 1;
	ADCON2bits.ADCS0 = 0;
	*/

	for(;;) {
		if( PORTBbits.RB0 == 0 ) { // RA0 input is ground
			DCCtrl(0.5);
			LATDbits.LATD4 = 0;//writes RA1 with 0
			LATDbits.LATD5 = 1;//writes RA1 with 1
		} else if( PORTBbits.RB0 == 1 ){ //RA0 input is high
			
			DCCtrl(0.0);
			LATDbits.LATD4 = 1;//writes RA1 with 1
			LATDbits.LATD5 = 0;//writes RA1 with 0
		}
	}
}

