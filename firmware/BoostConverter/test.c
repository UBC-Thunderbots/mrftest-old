#include <pic18f4550.h>
#include "pwm.h"

void main(void) {
	ADCON1 = 0x0F;//Setting all pins as digital
	TRISAbits.TRISA0 = 1;//RA0 set to analog input
	TRISAbits.TRISA2 = 1;//RA2 set to input
	TRISAbits.TRISA1 = 0;//RA1 set to output
	InitPWM();
	
	ADCON1bits.PCFG0 = 0;//setting RA0 as analog
	ADCON1bits.VCFG1 = 0;//setting voltage reference
	ADCON1bits.VCFG0 = 0;//setting voltage reference
	ADCON0bits.ADON = 1;//Turning on ADC
	//ADCON2bits.*/

	for(;;) {
		if( PORTAbits.RA0 == 0 ) { // RA0 input is ground
			DCCtrl(0.25);
			LATAbits.LATA1 = 0;//writes RA1 with 0

		} else if( PORTAbits.RA0 == 1 ){ //RA0 input is high
			
			DCCtrl(0.9);
			LATAbits.LATA1 = 1;//writes RA1 with 1
		}
	}
}

