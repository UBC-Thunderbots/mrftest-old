#include <pic18f4550.h>
#include "config.h"

#define Imax 10.0
#define Tstep 83.3e-9
#define L 2.2e-6
#define Vin 14.4

void main(void) {

	/* Standard C, declare all variables at the start of the function!*/

	unsigned char i;
	unsigned int reading;
	unsigned int offtime;
	float voltage;
/* PIN Setup */

	INTCON2bits.RBPU = 0; // Internal pullups for port B enabled
	TRISBbits.TRISB0 = 1;
	TRISBbits.TRISB1 = 1;//RB0 set to input
	TRISBbits.TRISB2 = 0;//RB0 set to input
	TRISBbits.TRISB3 = 0;//RB0 set to input
	LATB = 0x00;

	/* ADC Setup */

	ADCON0= 0x11;
	TRISA = 0x3F;
	TRISE = 0x01;
	ADCON1 = 0x09;
	
	ADCON2bits.ADFM = 1;//Right Justified
	//Now i need to set conversion time and acquisition times.
	ADCON2bits.ADCS2 = 1;//Tad = 64Tosc;
	ADCON2bits.ADCS1 = 1;//
	ADCON2bits.ADCS0 = 0;//
	ADCON2bits.ACQT2 = 0;//Tacq = 2Tad;
	ADCON2bits.ACQT1 = 0;//
	ADCON2bits.ACQT0 = 1;//
	ADCON0bits.ADON = 1;//Turning on ADC
	
	TRISCbits.TRISC2 = 0;	
	
	/* PWM Setup */
	for(;;) {
		
		// Reading Voltage
		ADCON0bits.GO = 1;
		while( ADCON0bits.GO == 1 ){}
		reading = ADRESH;
		reading = (reading<<8) + ADRESL;
		voltage = reading * 5.0 / 1023.0 * ( 220.0 + 2.2) / 2.2;
		
		if(voltage < 100){
			LATBbits.LATB3 = 0;
			if(!PORTBbits.RB0) {
				LATBbits.LATB2 = 1;
				for(i = 0; i< 10 i++) {
					LATCbits.LATC2 = 1;
					_asm
						nop;
						nop;
						nop;
						nop;
						nop;
						nop;
						nop;
						nop;
						nop;
						nop;
					_endasm;
					LATCbits.LATC2 = 0;
					offtime = Vin * 10 / (voltage + 0.7) * 11;\

					//embedded assembly for off time.
				}
			}	else {
				LATBbits.LATB2 = 0;
			}
		} else {
			LATBbits.LATB3 = 1;
		}
	}
}

