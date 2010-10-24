/* PWM generator cpp file */
/* Generates PWM with a constant on time for CCP1. On Time calculated from theoretical values */
/* Off time varies to match duty cycle input */

#include "pwm.h"
#include <pic18f4550.h>

#define ON_TIME 			0.0000176 //On time is 17.6e-6 **TEMPORARILY MADE 0.000003666s 
#define F_OSC 				48000000 //oscillator frequency

#define LSBS 				0x03
#define CCP1CON_LSBS 			0xCF

#define MIN_PWM 0.0515625
#define MAX_PWM_THRESHOLD 0.92

#define PRESCALER4 			4
#define PRESCALER16 			16

/*Timer2 prescaler flags*/
char PRESCALER16_FLAG = 2;
char PRESCALER4_FLAG = 2;

/*PWM initialization Function*/
void InitPWM() {
	CCP1CONbits.CCP1M0 = 0;//
	CCP1CONbits.CCP1M1 = 0;// 
	CCP1CONbits.CCP1M2 = 1;//
	CCP1CONbits.CCP1M3 = 1;//Configuring CCP for PWM
	CCP1CONbits.P1M1 = 0;//
	CCP1CONbits.P1M0 = 0;//

	T2CONbits.T2CKPS1 = 1;//16Prescaler
	TRISCbits.TRISC2 = 0; // setting RC2 pin for CCP1 output
	T2CONbits.TMR2ON = 1; // Timer 2 enabled
	//LATC = 0x00;// setting as output
}

/*Duty cycle control with on time at 17.6E-6s*/
void DCCtrl(float dutyCycle) {
	
	if( dutyCycle > MIN_PWM && dutyCycle <= MAX_PWM_THRESHOLD ) {
		unsigned char decPeriod;
		unsigned int decDCTime;
		unsigned char temp;
		if( dutyCycle > 0.825 ) {// To maximize resolution, duty cycles above 0.825 should have prescaler set up to 1
			if( PRESCALER16_FLAG != 0 || PRESCALER4_FLAG != 0 ) { // Prescaler Setting to 1
				T2CONbits.T2CKPS1 = 0;
				T2CONbits.T2CKPS0 = 0;				
				decDCTime = ON_TIME*F_OSC;
				temp = decDCTime & LSBS;
				temp <<= 4;
				CCP1CON = (CCP1CON & CCP1CON_LSBS) | temp;
				decDCTime >>= 2;
				decDCTime = decDCTime & 0XFF;
				CCPR1L = decDCTime;
				PRESCALER16_FLAG = 0;
				PRESCALER4_FLAG = 0;
			}
			decPeriod = ((ON_TIME*F_OSC)/(dutyCycle*4.0)) - 1; // PR2 in decimal form (controls period)
			PR2 = decPeriod;

		} else if( dutyCycle > .20625 ) {// To maximize resolution, duty cycles above 0.20625 and below 0.825 should have prescaler set up to 4
			if( PRESCALER4_FLAG != 1 ) { // Prescaler Setting to 4
				T2CONbits.T2CKPS1 = 0;
				T2CONbits.T2CKPS0 = 1;				
				decDCTime = ON_TIME*F_OSC/PRESCALER4;
				temp = decDCTime & LSBS;
				temp <<= 4;
				CCP1CON = (CCP1CON & CCP1CON_LSBS) | temp;
				decDCTime >>= 2;
				decDCTime = decDCTime & 0XFF;
				CCPR1L = decDCTime;
				PRESCALER4_FLAG = 1;
				PRESCALER16_FLAG = 0;
			}
			decPeriod = ((ON_TIME*F_OSC)/(PRESCALER4*dutyCycle*4.0)) - 1; // PR2 in decimal form (controls period)
			PR2 = decPeriod;
			

		} else {
			if( PRESCALER16_FLAG != 1 ) { // Prescaler Setting to 16
				T2CONbits.T2CKPS1 = 1;				
				decDCTime = ON_TIME*F_OSC/PRESCALER16;
				temp = decDCTime & LSBS;
				temp <<= 4;
				CCP1CON = (CCP1CON & CCP1CON_LSBS) | temp;
				decDCTime >>= 2;
				decDCTime = decDCTime & 0XFF;
				CCPR1L = decDCTime;
				PRESCALER16_FLAG = 1;
				PRESCALER4_FLAG = 0;
			}
			decPeriod = ((ON_TIME*F_OSC)/(PRESCALER16*dutyCycle*4.0)) - 1; // PR2 in decimal form (controls period)
			PR2 = decPeriod;

		}

	} else {		
		if( PRESCALER16_FLAG != 2 || PRESCALER4_FLAG != 2 ) { // Prescaler Setting to 1
			T2CONbits.T2CKPS1 = 0;
			T2CONbits.T2CKPS0 = 0;
			CCP1CONbits.DC1B0 = 0;
			CCP1CONbits.DC1B1 = 0;
			PRESCALER16_FLAG = 2;
			PRESCALER4_FLAG = 2;
		}
		CCPR1L = 0x00;// Dutycycle set to zero if duty cycle is beyond allowable range
	}

}

