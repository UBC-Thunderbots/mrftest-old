/* Main Boost Converter file */
/* By: Lok Tin Lam */

#include <pic18f4550.h>
#include "pwm.h"

#define V_DIODE 0.7
#define V_SOURCE 15.0
#define V_REF 5.0
#define V_SET_POINT 240.0
#define EIGHT_BIT 256.0
#define ADC_RATIO 100.0
#define TEN_BIT 1023.0
#define SAFETY_FACTOR 1.1
/* Boost Converter Control Algorithm implemented */
/* Kp at 200V */

void main(void) {

	/* Standard C, declare all variables at the start of the function!*/

	
	unsigned char adresH;
	unsigned char adresL;
	float dutyCycle1;//Soft Start Duty Cycle
	float dutyCycle2;//Proportional Control Duty Cycle
	float dutyCycle;//Implemented Duty Cycle
	float Kp = 0.046;//Proportional Constant	**Calculated to switch over from soft start control at 220V, given V_SOURCE and V_SET _POINT**
	float fMsb  = EIGHT_BIT;
	float Vout;
	float Vcap;	
	float Vtest;
	/* PIN Setup */

	ADCON1 = 0x0F;//Setting all pins as digital
	INTCON2bits.RBPU = 0; // Internal pullups for port B enabled
	TRISBbits.TRISB0 = 1;//RB0 set to input
	TRISDbits.TRISD4 = 0;//Red LED
	TRISDbits.TRISD5 = 0;//Green LED 
	TRISDbits.TRISD6 = 0;//Green LED
	TRISAbits.TRISA1 = 1;//setting RA1 as input for external device control						
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
		
		// Reading Voltage
		ADCON0bits.GO = 1;
		while( ADCON0bits.GO == 1 ){}
		adresH = ADRESH;
		adresL = ADRESL;

		// Producing capacitor voltage
		Vtest = (adresH*fMsb + adresL)*V_REF/TEN_BIT;
		Vcap = Vtest*ADC_RATIO;

		if( Vcap > 250 ) {// debugging check to make sure ADC is working
			LATDbits.LATD4 = 1;
		} else {
			LATDbits.LATD4 = 0;
		}
		if( ( PORTBbits.RB0 == 0 || PORTAbits.RA1 == 1 ) && Vcap < 235 ) {// if button is pushed or if set point is beyond 240V, turn controller to zero
			LATDbits.LATD5 = 1;
			Vout = Vcap + V_DIODE;

			dutyCycle1 = 1-(V_SOURCE/Vout);// Soft start control
			dutyCycle2 = Kp*(V_SET_POINT-Vout);// Proportional Control

			dutyCycle = (dutyCycle1 <= dutyCycle2) ? dutyCycle1 : dutyCycle2;// Choosing smaller of two duty cycle values
			if(dutyCycle < 0.06) {
				dutyCycle = 0.06*SAFETY_FACTOR;
			}
			DCCtrl(dutyCycle/SAFETY_FACTOR);
		} else {
			LATDbits.LATD5 = 0;
			DCCtrl(0.0);
		}
	}
}

