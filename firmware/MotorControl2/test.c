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

char drive_table_forward[8] = 
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

char decceleration_drive_table[8] = 
{
	// RD7 = 3P, RD6 = 3N, RD5 = 2P, RD4 = 2N, RD3 = 1P, RD2 = 1N
	
	// 000 (shouldn't happen)
	BASE,
	
	// 001
	BASE ^ TOGGLE_1P ^ TOGGLE_3P,
	
	// 010
	BASE ^ TOGGLE_1P ^ TOGGLE_2P,
	
	// 011
	BASE ^ TOGGLE_2P ^ TOGGLE_3P,
	
	// 100
	BASE ^ TOGGLE_3P ^ TOGGLE_2P,
	
	// 101
	BASE ^ TOGGLE_1P ^ TOGGLE_2P,
	
	// 110
	BASE ^ TOGGLE_3P ^ TOGGLE_1P,
	
	// 111 (shouldn't happen)
	BASE
};

unsigned char read_adc()
{
	ADCON0bits.GO = 1;

	while (ADCON0bits.GO);
	
	return ADRESH;

}

void main(void)
{
	unsigned char hall_state;
		
	unsigned char duty_cycle = read_adc();
	
	unsigned char run;

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
	
	while (1)
	{
		if (!ADCON0bits.GO)
		{
			duty_cycle = run ? ADRESH : 0;
			ADCON0bits.GO = 1;
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
		
		if (TMR0L >= duty_cycle)
		{
			LATD = BASE;
		}
		else
		{		
			LATD = drive_table_forward[hall_state];		
		}
		
		if (INTCONbits.TMR0IF)
		{
			INTCONbits.TMR0IF = 0;
		}
		
	}
}

