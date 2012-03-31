#include <pic18f4550.h>

static const char __code __at(__CONFIG1L) c1l = _USBPLL_CLOCK_SRC_FROM_96MHZ_PLL_2_1L & _CPUDIV__OSC1_OSC2_SRC___1__96MHZ_PLL_SRC___2__1L & _PLLDIV_NO_DIVIDE__4MHZ_INPUT__1L;//SET SYSTEM SYSTEM CLOCK TO RUN AT 96MHz/2 WITH NO PRESCALER
static const char __code __at(__CONFIG1H) c1h = _OSC_XT__XT_PLL__USB_XT_1H & _FCMEN_OFF_1H & _IESO_OFF_1H;//XTPLL with no fail safe monitor
//static const char __code __at(__CONFIG1H) c1h = _OSC_XT__XT_PLL__USB_XT_1H & _FCMEN_ON_1H & _IESO_OFF_1H;//XTPLL enabled (0011) with fail safe monitor enable bit
static const char __code __at(__CONFIG2L) c2l = _VREGEN_OFF_2L & _PUT_ON_2L & _BODEN_OFF_2L;
static const char __code __at(__CONFIG2H) c2h = _WDT_DISABLED_CONTROLLED_2H;
static const char __code __at(__CONFIG3H) c3h = _CCP2MUX_RC1_3H & _PBADEN_PORTB_4_0__CONFIGURED_AS_DIGITAL_I_O_ON_RESET_3H & _MCLRE_MCLR_ON_RE3_OFF_3H;//MCLR pin on,CCP2 output mux set to RC1
static const char __code __at(__CONFIG4L) c4l = _STVR_ON_4L & _LVP_OFF_4L & _ENICPORT_OFF_4L & _ENHCPU_OFF_4L & _BACKBUG_OFF_4L;
static const char __code __at(__CONFIG5L) c5l = _CP_0_OFF_5L & _CP_1_OFF_5L & _CP_2_OFF_5L & _CP_3_OFF_5L;
static const char __code __at(__CONFIG5H) c5h = _CPD_OFF_5H & _CPB_OFF_5H;
static const char __code __at(__CONFIG6L) c6l = _WRT_0_OFF_6L & _WRT_1_OFF_6L & _WRT_2_OFF_6L & _WRT_3_OFF_6L;
static const char __code __at(__CONFIG6H) c6h = _WRTD_OFF_6H & _WRTB_OFF_6H & _WRTC_OFF_6H;
static const char __code __at(__CONFIG7L) c7l = _EBTR_0_OFF_7L & _EBTR_1_OFF_7L & _EBTR_2_OFF_7L & _EBTR_3_OFF_7L;
static const char __code __at(__CONFIG7H) c7h = _EBTRB_OFF_7H;

#define TOGGLE_1P 0x80; //10000000
#define TOGGLE_1N 0x40; //01000000
#define TOGGLE_2P 0x20; //00100000
#define TOGGLE_2N 0x10; //00010000
#define TOGGLE_3P 0x08; //00001000
#define TOGGLE_3N 0x04; //00000100


#define BASE (TOGGLE_3P | TOGGLE_2P | TOGGLE_1P)


char drive_table[8] = 
{
	// RF7 = 1P, RF6 = 3N, RF5 = 2P, RF4 = 2N, RF3 = 1P, RF2 = 1N
	
	// 000 (shouldn't happen)
	BASE,   
	
	// 001 
	BASE ^ TOGGLE_1P ^ TOGGLE_3N,
	
	// 010
	BASE ^ TOGGLE_1N ^ TOGGLE_2P,
	
	// 011
	BASE ^ TOGGLE_2P ^ TOGGLE_3N,
	
	// 100
	BASE ^ TOGGLE_3P ^ TOGGLE_2N,
	
	// 101
	BASE ^ TOGGLE_1P ^ TOGGLE_2N,
	
	// 110
	BASE ^ TOGGLE_3P ^ TOGGLE_1N,
	
	// 111 (shouldn't happen)
	BASE
};

unsigned char forward_hall_state[8] =
{
	0, //000
	3, //001
	6, //010
	2, //011
	5, //100
	1, //101
	4, //110
	0, //111	
};

unsigned char backward_hall_state[8] =
{
	0, //000
	5, //001
	3, //010
	1, //011
	6, //100
	4, //101
	2, //110
	0, //111	
};

void lcdinit ()
{
	//E=0 RS=1 RW =1
	LATD = 0x3c;    //00111100;
	wait2ms();
	LATD = 0x3c;    //00111100;
	wait2ms();
	LATD = 0x0c;    //00001100;
	wait2ms();
	LATD = 0x01;    //00000001;
	wait2ms();
	LATD = 0x06;    //00000110;
	wait2ms();
	
}

void wait2ms()
{
	while(!INTCONbits.TMR0IF);
	INTCONbits.TMR0IF = 0;
	TMR0L = 0x10;   //00010000

	while(!INTCONbits.TMR0IF);
	INTCONbits.TMR0IF = 0;
}

void lcdwrite (char print)
{
	//E=1 RS=0 RW=1	
	LATD = print;
}

void lcdwriteword (char print[])
{	
	int i;	
	for (i = 0; i<sizeof(print); ++i)
		lcdwrite(print[i]);
}

void main (void)
{
	int knob;	
	int j;
	int k;

	/* startup delay */
	for (j = 0; j < 1000; ++j)
	{
		for (k = 0; k < 1000; ++k);
	}

	TRISC = 0x1f;   //00011111;   	
	TRISD = 0;      //Output
	TRISF = 0;      //Output
	TRISE = 0x70;   //01110000
	
	
	// turn on ADC
	ADCON0 = 0x05; //00000101;   Channel 1, On
	ADCON1 = 0x2e; //00101110;   Left justified, only 8 bits needed
	ANCON0 = 0x98; //10011000;   Set AN0-2 as analog
	ADCON0bits.GO = 1; // start ADC
	
	
	// 8-bit counter with prescalar 4
	TMR0L = 0xfb; //11111011
	T0CON = 0xc8; //11001000


	//initialization
	LATE |= 0x06; //00000110
	LATE &= 0xf6; //11110110

	LATCbits.LATC5 = 1;
	LATCbits.LATC6 = 0;
	
	//clock init

	//lcdinit();

	/*while (1)
	{

		unsigned char hall_state;
		
		signed int duty_cycle = 0;
	
		unsigned char run;
	
		unsigned int speed_window_divider_counter = 0;
	
		signed char speed = 0;
	
		signed char speed_counter = 0;

		int speed_setpoint = 0;
	
		unsigned char old_hall_state;
	
		int speed_scaled = 0;
		
		if (!ADCON0bits.GO)
		{
			if(ADRESH > 0xD2)
				LATEbits.LATE3 = 0;   //Turn off FETS
			ADCON0 = 0x09; //00000001 For POT
			ADCON0bits.GO = 1; //start ADC
		}
		
		
		//while (1)
		//{
		//	if (!ADCON0bits.GO)
		//	{
		//		knob = ADRESH;
		//		ADCON0 = 0x05; //00000001 back to mosfets
		//		ADCON0bits.GO = 1; //start ADC
		//		break;
		//	}
		//}

		
		//check buttons, change LCD in case
		if (!LATEbits.LATE6)
		{
			++duty_cycle;
		{

		if (!LATEbits.LATE5)
		{
			--duty_cycle;
		{
		
		
		hall_state = PORTC & 0x07;	

		if (hall_state != old_hall_state)
		{
			if (hall_state == forward_hall_state[old_hall_state])
			{
				++speed_counter;
			}
			else if (hall_state == backward_hall_state[old_hall_state])
			{
				--speed_counter;
			}			
			
			old_hall_state = hall_state;
		}

		if (TMR0L >= int_abs(duty_cycle))
		{
			LATD = BASE;
		}
		else
		{	
			if (duty_cycle > 0)
			{
				LATD = drive_table[hall_state];		
			}
			else
			{
				LATD = drive_table[hall_state ^ 0x07];	
			}
		}
		
		if (INTCONbits.TMR0IF)  //pcl
		{
			INTCONbits.TMR0IF = 0;

			TMR0L = 0xfb;  //11111011;
						
		/*	++speed_window_divider_counter;
			
			if (speed_window_divider_counter > SPEED_WINDOW_PRESCALAR)
			{
				speed_window_divider_counter = 0;
				
				speed = speed_counter;
				
				speed_counter = 0;
				
				
				 speed is -32 to 32 	
				speed_scaled = speed * SPEED_SCALE; // -127 to 127
							
				if (speed_scaled < (speed_setpoint - 10) || speed_scaled > (speed_setpoint + 10))
				{
					duty_cycle = speed_setpoint - speed_scaled;
				}
				else
				{	
					duty_cycle = 0;
				}
			}
			
			
		}
	}  */ 

}



