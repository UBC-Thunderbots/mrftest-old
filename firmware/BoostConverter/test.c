#include <pic18f4550.h>
#include "pwm.h"

#define VREF 5.0
#define EIGHT_BIT 256.0
#define ADC_RATIO 100.0
#define TEN_BIT 1023.0

/* Use ADC input to control duty cycle of PWM */
/* Implementation INCOMPLETE */

void main(void) {

	/* Standard C, declare all variables at the start of the function!*/

	unsigned char binADCVal;
	float decADCVal;
	float dutyCycle;
	unsigned char adresH;
	unsigned char adresL;
	float fMsb  = EIGHT_BIT;
	float Vout;
	

	/* PIN Setup */

	ADCON1 = 0x0F;//Setting all pins as digital
	INTCON2bits.RBPU = 0; // Internal pullups for port B enabled
	TRISBbits.TRISB0 = 1;//RB1 set to input
	TRISDbits.TRISD4 = 0;//Red LED
	TRISDbits.TRISD5 = 0;//Green LED 
	TRISDbits.TRISD6 = 0;//Green LED
	LATD = 0x00;


	/* ADC Setup */

	ADCON1bits.PCFG0 = 0;//setting RA0 as analog
	TRISAbits.TRISA0 = 1;//setting RA0 as input
	ADCON1bits.VCFG1 = 0;//setting voltage reference
	ADCON1bits.VCFG0 = 0;//setting voltage reference
	ADCON2bits.ADFM = 1;//Right Justified

	//Now i need to set conversion time and acquisition times.
	ADCON2bits.ADCS2 = 1;//Tad = 64Tosc;
	ADCON2bits.ADCS1 = 1;//
	ADCON2bits.ADCS0 = 0;//
	ADCON2bits.ACQT2 = 0;//Tacq = 2Tad;
	ADCON2bits.ACQT1 = 0;//
	ADCON2bits.ACQT0 = 1;//
	ADCON0bits.ADON = 1;//Turning on ADC
	
	/* PWM Setup */
	
	InitPWM();



	/* Main Loop */

	for(;;) {

		ADCON0bits.GO = 1;
		while( ADCON0bits.GO == 1 ){}
		adresH = ADRESH;
		adresL = ADRESL;
		Vout = (adresH*fMsb + adresL)*VREF/TEN_BIT;
		
		
		/*Code for 10 bit adc resolution, note a type change to binADCVal is required*/

/*		decADCVal = VREF*(binADCVal/EIGHT_BIT);*/
		
		dutyCycle = Vout/VREF;
		DCCtrl(dutyCycle);

		if( Vout > 2.5 )
			LATDbits.LATD5 = 0;
		else
			LATDbits.LATD5 = 1;


		/*if( PORTBbits.RB0 == 0 ) { // RA0 input is ground
			DCCtrl(0.5);
			LATDbits.LATD4 = 0;//writes RA1 with 0
			LATDbits.LATD5 = 1;//writes RA1 with 1
		} else if( PORTBbits.RB0 == 1 ){ //RA0 input is high
			
			DCCtrl(0.0);
			LATDbits.LATD4 = 1;//writes RA1 with 1
			LATDbits.LATD5 = 0;//writes RA1 with 0
		}*/
	}
}

