#include <pic18f4550.h>

#include <pic16/delay.h>

#define V_DIODE 0.7
#define V_SOURCE 15.0
#define V_REF 5.0
#define EIGHT_BIT 256.0
#define ADC_RATIO 100.0
#define TEN_BIT 1023.0

#define V_CAP_HIGH_THRES 220.0f
#define V_CAP_LOW_THRES 200.0f
#define V_CAP_LOW_SAFETY_THRES 10.0f

void main(void) {

	/* Standard C, declare all variables at the start of the function!*/	
	unsigned char adresH;
	unsigned char adresL;
	
	float fMsb  = EIGHT_BIT;
	float Vout;
	float Vcap;	
	float Vtest;
	
	unsigned char charging = 0; // program state, bool
	
	/* PIN Setup */
	ADCON1 = 0x0F;//Setting all pins as digital
	//INTCON2bits.RBPU = 0; // Internal pullups for port B enabled
	TRISDbits.TRISD2 = 1;//RD2 set to input
	TRISDbits.TRISD4 = 0;//Red LED
	TRISDbits.TRISD5 = 0;//Green LED 
	TRISDbits.TRISD6 = 0;//Green LED
	LATD = 0x00;
	
	// charge output
	TRISCbits.TRISC2 = 0;


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
		
		if (charging)
		{
			if (Vcap < V_CAP_HIGH_THRES && Vcap > V_CAP_LOW_SAFETY_THRES && PORTDbits.RD2 == 0)
			{
				// keep charging
				LATCbits.LATC2 = 0;
				delay10tcy(2); // 1.67us on time (20 instructions)
				LATCbits.LATC2 = 1;
				delay100tcy(10); // 83us (1000 instructions) + ADC delay off time
			}
			else
			{
				charging = 0;
			}
		}
		else
		{
			if (PORTDbits.RD2 == 0 && Vcap < V_CAP_LOW_THRES)
			{
				charging = 1;
			}
		}
	}
}

