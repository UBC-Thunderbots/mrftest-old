#include "format.h"
#include <gpio.h>
#include <init.h>
#include <rcc.h>
#include <registers/exti.h>
#include <registers/flash.h>
#include <registers/nvic.h>
#include <registers/timer.h>
#include <registers/power.h>
#include <registers/scb.h>
#include <registers/syscfg.h>
#include <registers/systick.h>
#include <sleep.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unused.h>

volatile uint8_t G_status = 0; // 1 for first triggered, 2 for second triggered, 0 for last wrap_count is output to screen

static void stm32_main(void) __attribute__((noreturn));
static void nmi_vector(void);
static void hard_fault_vector(void);
static void memory_manage_vector(void);
static void bus_fault_vector(void);
static void usage_fault_vector(void);
static void service_call_vector(void);
static void pending_service_vector(void);
static void system_tick_vector(void);
static void external_interrupt_15_10_vector(void);
static void timer2_wrap_interrupt(void);

static void display( float );
static void LCD_write( char a );

static void tic(void);
static void toc(void);


static char mstack[65536] __attribute__((section(".mstack")));

typedef void (*fptr)(void);
static const fptr exception_vectors[16] __attribute__((used, section(".exception_vectors"))) = {
	// Vector 0 contains the reset stack pointer
	[0] = (fptr) (mstack + sizeof(mstack)),
	// Vector 1 contains the reset vector
	[1] = &stm32_main,
	// Vector 2 contains the NMI vector
	[2] = &nmi_vector,
	// Vector 3 contains the HardFault vector
	[3] = &hard_fault_vector,
	// Vector 4 contains the MemManage vector
	[4] = &memory_manage_vector,
	// Vector 5 contains the BusFault vector
	[5] = &bus_fault_vector,
	// Vector 6 contains the UsageFault vector
	[6] = &usage_fault_vector,
	// Vector 11 contains the SVCall vector
	[11] = &service_call_vector,
	// Vector 14 contains the PendSV vector
	[14] = &pending_service_vector,
	// Vector 15 contains the SysTick vector
	[15] = &system_tick_vector,
};

static const fptr interrupt_vectors[82] __attribute__((used, section(".interrupt_vectors"))) = {

	[28] = &timer2_wrap_interrupt,
	[40] = &external_interrupt_15_10_vector,
};

static void nmi_vector(void) {
	for (;;);
}

static void hard_fault_vector(void) {
	for (;;);
}

static void memory_manage_vector(void) {
	for (;;);
}

static void bus_fault_vector(void) {
	for (;;);
}

static void usage_fault_vector(void) {
	for (;;);
}

static void service_call_vector(void) {
	for (;;);
}

static void pending_service_vector(void) {
	for (;;);
}

static void system_tick_vector(void) {
	for (;;);
}


// This function is where the state machine goes
static void external_interrupt_15_10_vector(void){
	// TODO
	// Take one of the interrupt to clear every thing and start the timer, set status_timer_on to on.
	// Take the other interrupt to stop the timer and update display

	if( EXTI_PR & (1<<12)){
		EXTI_PR = 1<<12;
		gpio_toggle(GPIOB, 0);
		if( G_status == 0 ){
			G_status = 1;
			tic();
		}
		//tic();
	}

	if( EXTI_PR & (1<<13)){
		EXTI_PR = 1<<13;
		gpio_toggle(GPIOB, 1);
		if( G_status == 1 ){
			G_status = 2;
			toc();
		}
		//toc();
	}
	
}

static const init_specs_t INIT_SPECS = {
	.flags = {
		.hse_crystal = true,
		.freertos = false,
	},
	.hse_frequency = 8,
	.pll_frequency = 288,
	.sys_frequency = 144,
	.cpu_frequency = 144,
	.apb1_frequency = 36,
	.apb2_frequency = 72,
	.exception_core_writer = 0,
	.exception_app_cbs = {
		.early = 0,
		.late = 0,
	},
};

/***********************************************************
 *    		 	Timing Functions		   *
 ***********************************************************/


volatile uint32_t wrap_count;

static void tic_toc_setup(void){
	rcc_enable(APB1, TIM2);
	rcc_reset(APB1, TIM2);

	{
		TIM2_5_CR1_t tmp = {
			.CKD = 0, // Timer runs at full clock frequency.
			.ARPE = 0, // Auto-reload register is not buffered.
			.CMS = 0, // Counter counts in one direction.
			.DIR = 0, // Counter counts up.
			.OPM = 0, // Counter counts forever.
			.URS = 0, // Counter overflow, UG bit set, and slave mode update generate an interrupt.
			.UDIS = 0, // Updates to control registers are allowed.
			.CEN = 0, // Counter is not counting right now.
		};
		TIM2.CR1 = tmp;
	}

	{
		TIM2_5_CR2_t tmp = {
			.TI1S = 0, // TIM2_CH1 pin is connected to TI1.
			.MMS = 0, // TIM2.EGR.UG is sent as output to slave timers.
			.CCDS = 0, // If DMA is enabled, requests are sent when CC events occur.
		};
		TIM2.CR2 = tmp;
	}

	{
		TIM2_5_SMCR_t tmp = { 0 }; // No external triggers or slave synchronization.
		TIM2.SMCR = tmp;
	}

	{
		TIM2_5_DIER_t tmp = {
			.UIE = 1, // Enable interrupt on timer update
		};
		TIM2.DIER = tmp; // Enable interrupt on timer update
	}

	TIM2.PSC = 0b10000000;
	
	TIM2.ARR = 0x40;
	
	NVIC_ISER[28 / 32] = 1 << (28 % 32);
}

static void timer2_wrap_interrupt(void){
	wrap_count++;
	{
		TIM2_5_SR_t tmp = { 0 }; // Clear interrupt flag.
		TIM2.SR = tmp;
	}
}

static void tic(void){
	// clear counter, by setting UG
	TIM2.EGR.UG = 1;
	// enable clock, by setting CEN
	TIM2.CR1.CEN = 1;
	wrap_count = 0;
}

static void toc(void){
	// stop clock
	TIM2_5_CR1_t tmp = { 0 };
	TIM2.CR1 = tmp;
}

/***********************************************************
 *    util functions when for the lcd control output       *
 ***********************************************************/

// pin assignment
#define PIN_E 15
#define PIN_RW 14
#define PIN_RS 13

// line states
#define LCD_READ true
#define LCD_WRITE false
#define LCD_COMMAND false
#define LCD_DATA true

// byte command that doesn't vary
#define LCD_CLEAR_SCREEN 0x00000001
#define LCD_HOME_SCREEN 0x00000010

// byte command that this prefer
#define LCD_FUNCTION_SET_P	0x00111100	// display on, two line mode
#define LCD_ON_CONTROL_P		0x00001100	// display one everything else off
#define LCD_ENTRY_MODE_P	0x00000110	// increment, entire shift off

// change lcd mode
static void LCD_switch_mode( bool signal_rs, bool signal_rw ){
	gpio_set_output(GPIOC, PIN_RS, signal_rs);
	gpio_set_output(GPIOC, PIN_RW, signal_rw);
}

// write 
static void LCD_write( char a ){
	gpio_set_reset_mask(GPIOC, a, 0xFF);
	LCD_switch_mode( LCD_DATA, LCD_WRITE );
	gpio_set(GPIOC, PIN_E);
	sleep_us(1);
	gpio_reset(GPIOC, PIN_E);
	sleep_us(50);
}

static void LCD_command( char a ){
	gpio_set_reset_mask(GPIOC, a, 0xFF);
	LCD_switch_mode( LCD_COMMAND, LCD_WRITE );
	gpio_set(GPIOC, PIN_E);
	sleep_us(1);
	gpio_reset(GPIOC, PIN_E);
	sleep_us(1);
	
}

// initialize screen
static void LCD_init_routine(){
	sleep_ms(500); // recommanded waiting time is 40ms
	LCD_command( 0x30 );
	sleep_us(30);
	LCD_command( 0x30 );
	sleep_us(10);
	LCD_command( 0x30 );
	sleep_us(10);
	LCD_command( 0x38 ); // function set
	sleep_us(50);
	LCD_command( 0x1c ); // set cursor
	sleep_us(50);
	LCD_command( 0x0c ); // display on, cursor on
	sleep_us(50);
	LCD_command( 0x06 ); // entry mode set
	sleep_us(50);
	LCD_command( 0x02 ); // return home
	sleep_ms(2);
/*	sleep_us(50);
	LCD_command( LCD_ON_CONTROL_P );
	sleep_us(50);
	LCD_command( LCD_CLEAR_SCREEN );
	sleep_ms(2);
	LCD_command( LCD_ENTRY_MODE_P );
*/	sleep_us(50);
}

// write something to the screen, this writes all ones
static void LCD_write_something(){
	LCD_write( '1' );
	LCD_write( '2' );
	LCD_write( '3' );
	LCD_write( 'A' );
	LCD_write( 'B' );
	LCD_write( 'C' );
}

static void LCD_print( char* a, unsigned int size ){
	unsigned int i = 0;
	for( i = 0; i < size; i++){
		LCD_write( a[i] );
	}
}

/***************************************************
 *		Print functions			   *
 ***************************************************/


// print to lcd screen, starting from the firstline
/*static void LCD_print( char* a, int size ){
	int i=0;
	LCD_clear_screen();
	for( i = 0; i<size; i++ ){
		LCD_write_char(a[i]);
	}
}*/

/***************************************************
 *	Math: turning float to char		   *
 ***************************************************/



static int ftoi_single ( float fl ){
	static int i = 0;
	for( i = 1; i < 10; i++ ){
		if( fl < i ){
			return i-1;
		}
	}
	return 0;
}

// turn to scientific notation, with 10 significant figures
static void ftoa_sci ( float fl, char* a, int* dec ) {
	//static const int SIGFIG = 5;
	static unsigned int decimal_marker = 0;
	static int digit = 0;
	//int digits[4]={-1,-1,-1,-1};
	static unsigned int i = 0, j = 0;
	/*while (fl > 10){
		fl=fl/10;
		decimal_marker++;
	}*/

	/*for( i = 0; i< 5; i++ ){
		if( fl > 1){
			break;
		}
		fl=fl*0.1;
		decimal_marker++;
	}*/
	
	/*if( dec != 0 ){
		*dec = decimal_marker;
	}*/
	

	/*if( i == decimal_marker ){
		a[i] = ',';
		i++;
	}*/

	a[0] = (char)ftoi_single(fl);
	a[0] += 48;
	fl = (fl-ftoi_single(fl))*10;
	
	//a[0]='i';

	return;
}

// output float to char array with dynamic scaling. The output is in the form of xx.xxxk, x.xxxx or xxx.xm
static void ftoa_tho (float fl, char* a, int size){
	char sci_a[10];
	int decimal_mark;
	int num1, num2;
	int i, j; // i represents the index of char array a, j represents the index of char array sci_a. There is a bit of shuffling here for inserting the decimal point.

	ftoa_sci(fl,sci_a,&decimal_mark);
	switch( decimal_mark ){
		case 	-6:
		case 	-5:
		case	-4:	a[size-1]='u'; break;
		case	-3:	
		case	-2:	
		case	-1:	a[size-1]='m';	break;
		case	0:
		case	1:
		case	2:	a[size-1]=' ';	break;
		case	3:
		case	4:
		case	5:	a[size-1]='k';	break;
		default	:	a[size-1]='x'; 	break;
	}
	num1 = (decimal_mark+12)%3;
	num2 = num1 +1; // the index of a where the decimal point should be.
	for( i = size-2, j=size-3; i>=0; i--, j-- ){
		if( i == num2 ){
		// then we insert the decimal point
			a[i]='.';
			i--;
			continue;
		}
		a[i]=sci_a[j];
	}
	return;
}


/***************************************************
 *			Main			   *
 ***************************************************/

/*char buffer[10];
char test_c[]={'1','2','3','4','a'};
char* char_ptr = &buffer[0];
float test_num = 1.234;
int* ptr = 0;*/

static void stm32_main(void) {
	int counter_i = 0;
	char buffer[10];
	char test_c[]={'1','2','3','4','a'};
	char* char_ptr = &buffer[0];
	//float test_num = 1.234;
	int* ptr = 0;	
	uint32_t test_num = 123456;

	// Initialize chip
	init_chip(&INIT_SPECS);

	// Set up pins
	static const gpio_init_pin_t GPIO_INIT_PINS[4U][16U] = {
		{
			// PA0 = shorted to VDD
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
			// PA1 = shorted to VDD
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
			// PA2 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PA3 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PA4 = emitter 1 control (to P2)
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
			// PA5 = emitter 2 control (to P3)
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
			// PA6 = emitter 3 control (to P4)
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
			// PA7 = emitter 4 control (to P5)
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 1, .af = 0 },
			// PA8 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PA9 = OTG FS VBUS
			{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_PD, .od = 0, .af = 0 },
			// PA10 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PA11 = alternate function OTG FS
			{ .mode = GPIO_MODE_AF, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_25, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 10 },
			// PA12 = alternate function OTG FS
			{ .mode = GPIO_MODE_AF, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_25, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 10 },
			// PA13 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PA14 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PA15 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		},
		{
			// PB0 = indicator LED D2 (left)
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB1 = indicator LED D3 (right)
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB2 = BOOT1, hardwired low
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB3 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB4 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB5 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB6 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB7 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB9 = shorted to VSS
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB8 = shorted to VSS
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB10 = switch SW3 (left)
			{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_PD, .od = 0, .af = 0 },
			// PB11 = switch SW2 (right)
			{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_PD, .od = 0, .af = 0 },
			// PB12 = receiver 1 (from K4)
			{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB13 = receiver 2 (from K3)
			{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB14 = receiver 3 (from K2)
			{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PB15 = receiver 4 (from K1)
			{ .mode = GPIO_MODE_IN, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		},
		{
			// PC0 = LCD D0
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC1 = LCD D1
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC2 = LCD D2
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC3 = LCD D3
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC4 = LCD D4
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC5 = LCD D5
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC6 = LCD D6
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC7 = LCD D7
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC8 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC9 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC10 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC11 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC12 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC13 = LCD RS
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC14 = LCD RW
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PC15 = LCD E
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		},
		{
			// PD0 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD1 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD2 = N/C
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD3 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD4 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD5 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD6 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD7 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD8 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD9 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD10 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD11 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD12 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD13 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD14 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
			// PD15 = unimplemented on package
			{ .mode = GPIO_MODE_OUT, .otype = GPIO_OTYPE_PP, .ospeed = GPIO_OSPEED_2, .pupd = GPIO_PUPD_NONE, .od = 0, .af = 0 },
		},
	};
	gpio_init(GPIO_INIT_PINS);

	// setup interrupt
	rcc_enable(APB2, SYSCFG);
	rcc_reset(APB2, SYSCFG);
	SYSCFG_EXTICR[3] = 0b0001000100010001;
	rcc_disable(APB2, SYSCFG);
	//           ooo|ooo|ooo|ooo|
	EXTI_IMR = 0b0011000000000000;
	EXTI_FTSR= 0b0011000000000000;
	NVIC_ISER[40 / 32] = 1 << (40 % 32); 

// SETENA67 = 1; enable USB FS interrupt

	gpio_reset(GPIOC, PIN_E);


	// Wait a bit
	sleep_ms(100);

	// Turn on LED
	//GPIOB_BSRR = 3;
	//sleep_ms(1000);
	//GPIOB_BSRR = (3<< 16);
	//sleep_ms(10);
	// Initialize USB
	//usb_ep0_set_global_callbacks(&DEVICE_CBS);
	//usb_ep0_set_configuration_callbacks(CONFIG_CBS);
	//usb_attach(&DEVICE_INFO);
	//NVIC_ISER[67 / 32] = 1 << (67 % 32); // SETENA67 = 1; enable USB FS interrupt

	// Handle activity
	tic_toc_setup();
	/*tic();
	sleep_ms(3000);
	toc();*/

	// turn off portC pin 13, turn on pin 14, 15
	//GPIOC_BSRR = 3 << 13;
	//GPIOC_BSRR = 1 << (13+16);
	//GPIOC_BSRR = 1 << (14+16);
	LCD_init_routine();
	//LCD_write_something();
	formatuint8( buffer, wrap_count );
	LCD_print( buffer, 8 );
	//LCD_print( test_c, 5 );
	//LCD_write( ftoi_single(1.234)+48 );

	// Turn on LED
	gpio_set(GPIOB, 1);
	//sleep_ms(1000);
	gpio_reset(GPIOB, 0);
	//sleep_ms(10);
	for (;;) {
		/*for( counter_i = 0; counter_i < 10; counter_i++){
			GPIOC_BSRR = digits[counter_i];
			sleep_ms(1000);
		}*/
		/*for( counter_i = 0; counter_i < 8; counter_i++ ){
			GPIOC_BSRR = 1 << counter_i;
			sleep_ms(1000);
			GPIOC_BSRR = 1 << (counter_i+16);
			sleep_ms(1000);
			
		}*/
		/*if( TIM2_CNT > 0xF0 ){
			GPIOB_BSRR = (3<<16);
			toc();
		}*/
		/*if(GPIOB_IDR & (1<<12)) {
			GPIOB_BSRR = (3);
		} else {
			GPIOB_BSRR = (3 << 16);
		}*/
		if( G_status == 2 ){
			if( wrap_count != 0 ){
				formatuint8( buffer, 1716000/wrap_count );
				LCD_command( 0x02 ); // return home
				sleep_ms(2) ;
				LCD_print( buffer, 8 );
			}
			G_status = 0;

		}
		sleep_ms(1);
		
		gpio_reset(GPIOD, 13);
		sleep_ms(1000);
		gpio_set(GPIOD, 13);
		sleep_ms(1000);
		
	}
}

