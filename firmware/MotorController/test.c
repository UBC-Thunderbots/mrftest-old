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

#define TOGGLE_3P (1 << 5)
#define TOGGLE_3N (1 << 6)
#define TOGGLE_2P (1 << 7)
#define TOGGLE_2N (1 << 4)
#define TOGGLE_1P (1 << 3)
#define TOGGLE_1N (1 << 2)

#define MASK_ON (1 << 3)
#define MASK_OFF (1 << 4)

#define MASK_POWER_LED (1 << 0)
#define MASK_RUN_LED (1 << 1)
#define MASK_UNUSED_LED (1 << 2)

#define BASE (TOGGLE_3P | TOGGLE_2P | TOGGLE_1P)

#define SPEED_SCALE 4

//128 high
//96 high
//88 high
//84 high
//82 high
//80 low
//64 low

#define SPEED_WINDOW_PRESCALAR 81

char drive_table[8] = 
{
	// RD7 = 3P, RD6 = 3N, RD5 = 2P, RD4 = 2N, RD3 = 1P, RD2 = 1N
	
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

unsigned char read_adc()
{
	ADCON0bits.GO = 1;

	while (ADCON0bits.GO);
	
	return ADRESH;

}

int int_abs(int a)
{
	return a > 0 ? a : -a;
}

void main(void)
{
	unsigned char hall_state;
		
	signed int duty_cycle = 0;
	
	unsigned char run;
	
	unsigned int speed_window_divider_counter = 0;
	
	signed char speed = 0;
	
	signed char speed_counter = 0;
	
	unsigned char adc_read;
	
	int speed_setpoint = 0;
	
	unsigned char old_hall_state;
	
	int speed_scaled = 0;
	
	int i;
	volatile long int j, k;
	
	/* startup delay */
	for (j = 0; j < 1000; ++j)
	{
		for (k = 0; k < 1000; ++k);
	}

	TRISB = 0xff;
	
	TRISD = 0;
	
	TRISC = 0;
	
	LATC |= MASK_POWER_LED;
	
	LATC &= ~MASK_UNUSED_LED;
	
	// turn on ADC
	ADCON0 = 0;
	ADCON0bits.ADON = 1;
	
	// select Vss and Vdd for vref- and vref+
	ADCON1bits.VCFG0 = 0;
	ADCON1bits.VCFG1 = 0;
	
	// only enable AN0
	ADCON1bits.PCFG0 = 0;
	ADCON1bits.PCFG1 = 1;
	ADCON1bits.PCFG2 = 1;
	ADCON1bits.PCFG3 = 1;
	
	// set left justified because we only need 8 bits
	ADCON2bits.ADFM = 0;
	
	// set Taq to 12 Tad, clock to Fosc/64
	ADCON2bits.ACQT2 = 1;
	ADCON2bits.ACQT1 = 0;
	ADCON2bits.ACQT0 = 1;
	ADCON2bits.ADCS2 = 1;
	ADCON2bits.ADCS1 = 1;
	ADCON2bits.ADCS0 = 0;
	
	// 8-bit counter with prescalar 4
	T0CONbits.TMR0ON = 1;
	T0CONbits.T08BIT = 1;
	T0CONbits.T0CS = 0;
	T0CONbits.PSA = 0;
	T0CONbits.T0PS2 = 0;
	T0CONbits.T0PS1 = 0;
	T0CONbits.T0PS0 = 1;
	
	run = 0;
	
	old_hall_state = 0;
	
	/*
	i = 0;
	while(1)
	{
		//LATD = drive_table[i];
		LATD = BASE ^ TOGGLE_1P ^ TOGGLE_2P ^ TOGGLE_3P;
		for (j = 0; j < 500000; ++j); 
		
		
		LATD = BASE;
		for (j = 0; j < 500000; ++j);

	}
	*/
	
	
	while (1)
	{
		if (!ADCON0bits.GO)
		{
			adc_read = run ? ADRESH : 128;
			ADCON0bits.GO = 1;
			
			speed_setpoint = adc_read; // 0 extension
			speed_setpoint -= 128;
		}
		
		if (!(PORTA & MASK_ON))
		{
			run = 1;			
		}
		else if (!(PORTA & MASK_OFF))
		{
			run = 0;
		}
		
		if (run)
		{
			LATC |= MASK_RUN_LED;
		}
		else
		{
			LATC &= ~MASK_RUN_LED;
		}
	
		hall_state = (PORTB >> 2) & 0x07; // LSB = 3, MSB = 1
		
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
		
		if (INTCONbits.TMR0IF)
		{
			INTCONbits.TMR0IF = 0;
						
			++speed_window_divider_counter;
			
			if (speed_window_divider_counter > SPEED_WINDOW_PRESCALAR)
			{
				speed_window_divider_counter = 0;
				
				speed = speed_counter;
				
				speed_counter = 0;
				
				if (speed > 32)
				{
					LATC |= MASK_UNUSED_LED;
				}
				else
				{
					LATC &= ~MASK_UNUSED_LED;
				}
				
				/* speed is -32 to 32 */	
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
		
	}
}

