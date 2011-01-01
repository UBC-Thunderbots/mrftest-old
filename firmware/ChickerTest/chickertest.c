#include <pic18f4550.h>
//#include "pwm.h"

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

unsigned int read_adc()
{
	unsigned int ret = 0;
	
	LATAbits.LATA5 = 0;

	// make sure buffer is empty
	if (SSPSTATbits.BF)
	{
		ret = SSPBUF;
	}
		
	// dummy write
	SSPBUF = 0;
	
	// read the 5 MSB (yes 5!! not 6)
	while (!SSPSTATbits.BF);
	
	ret = SSPBUF;
	ret <<= 8;
	
	// another dummy write
	SSPBUF = 0;
	
	// read the 7 LSB + 1 bit garbage (15-bit SPI...)
	while (!SSPSTATbits.BF);
		
	ret |= SSPBUF;
	
	// now ret looks like this, 000dddddddddddd0, adjust it
	ret >>= 1;
	
	ret &= 0b0000111111111111;
	
	LATAbits.LATA5 = 1;
	
	return ret;
}

void main(void) {

	volatile int i;

	SSPCON1bits.SSPEN = 0;
	SSPCON1bits.CKP = 0;
	
	// fosc/16 = 0001, fosc/64 = 0010
	SSPCON1bits.SSPM3 = 0;
	SSPCON1bits.SSPM2 = 0;
	SSPCON1bits.SSPM1 = 1;
	SSPCON1bits.SSPM0 = 0;	
	
	SSPCON1bits.SSPEN = 1;
	TRISAbits.TRISA5 = 0;

	TRISDbits.TRISD5 = 0;
	
	TRISCbits.TRISC7 = 0;
	
	TRISBbits.TRISB1 = 0;
	
	for(;;)
	{
		LATDbits.LATD5 = 0;
		if (read_adc() > 0b0000100000000000)
		{
			LATDbits.LATD5 = 1;
		}
		else
		{
			LATDbits.LATD5 = 0;
		}
		
		for (i = 0; i < 1000; ++i);
	}
}

