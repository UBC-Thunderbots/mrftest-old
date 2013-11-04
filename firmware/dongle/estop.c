#include "estop.h"
#include <rcc.h>
#include <gpio.h>
#include <registers/adc.h>
#include <registers/nvic.h>
#include <registers/timer.h>
#include <sleep.h>

#define NOT_BROKEN_THRESHOLD 200

static unsigned int not_broken_count = NOT_BROKEN_THRESHOLD;
static estop_t value = ESTOP_BROKEN;
static estop_change_callback_t change_cb = 0;

static void estop_start_sample(void) {
	ADC1.CR2.SWSTART = 1; // Start conversion
}

static void estop_finish_sample(void) {
	uint32_t reading = ADC1.DR.DATA;
	// EStop open (stop) produces nominal 1.086 V
	// EStop closed (run) produces nominal 2.242 V
	//
	// Dividing the space:
	// 0–0.543 → broken
	// 0.543–1.664 → open
	// 1.664–2.771 → closed
	// 2.771–3.3 → broken
	if (reading < (uint32_t) (0.543 / 3.3 * 4096)) {
		// Broken
		value = ESTOP_BROKEN;
	} else if (reading < (uint32_t) (1.664 / 3.3 * 4096)) {
		// Open
		value = ESTOP_STOP;
	} else if (reading < (uint32_t) (2.771 / 3.3 * 4096)) {
		// Closed
		value = ESTOP_RUN;
	} else {
		// Broken
		value = ESTOP_BROKEN;
	}
}

void timer7_interrupt_vector(void) {
	{
		TIM_basic_SR_t tmp = { 0 };
		TIM7.SR = tmp; // Clear interrupt flag
	}
	estop_start_sample();
}

void adc_interrupt_vector(void) {
	estop_t old = value;
	estop_finish_sample();
	if (value == ESTOP_BROKEN) {
		not_broken_count = 0;
	} else if (not_broken_count < NOT_BROKEN_THRESHOLD) {
		++not_broken_count;
		value = ESTOP_BROKEN;
	}
	if (value != old && change_cb) {
		change_cb();
	}
}

void estop_init(void) {
	// Send power to the switch
	gpio_set(GPIOB, 0);

	// Configure the ADC
	rcc_enable(APB2, ADC1);
	rcc_reset(APB2, ADC);
	{
		ADC_CR1_t tmp = {
			.EOCIE = 0, // End of conversion interrupt disabled.
			.AWDIE = 0, // Analogue watchdog interrupt disabled.
			.JEOCIE = 0, // End of injected conversion interrupt disabled.
			.SCAN = 0, // Scan mode disabled.
			.JAUTO = 0, // Automatic injected group conversion disabled.
			.DISCEN = 0, // Discontinuous mode on regular channels disabled.
			.JDISCEN = 0, // Discontinuous mode on injected channels disabled.
			.JAWDEN = 0, // Analogue watchdog on injected channels disabled.
			.AWDEN = 0, // Analogue watchdog on regular channels disabled.
			.RES = 0, // 12-bit resolution.
			.OVRIE = 0, // Overrun interrupt disabled.
		};
		ADC1.CR1 = tmp;
	}
	{
		ADC_CR2_t tmp = {
			.ADON = 1, // Enable ADC.
			.CONT = 0, // Single conversion per trigger.
			.DMA = 0, // DMA disabled.
			.ALIGN = 0, // Data right-aligned in buffer.
			.JEXTEN = 0, // No external trigger for injected channels to start converting.
			.JSWSTART = 0, // Do not start converting injected channels now.
			.EXTEN = 0, // No external trigger for regular channels to start converting.
			.SWSTART = 0, // Do not start converting regular channels now.
		};
		ADC1.CR2 = tmp;
	}
	sleep_us(3);
	ADC1.SMPR2.SMP9 = 1; // Sample time on channel 9 is 15 cycles.
	ADC1.SQR1.L = 0; // Regular conversion sequence has one channel.
	ADC1.SQR3.SQ1 = 9; // First regular channel to convert is 9.

	// Take one sample now
	estop_start_sample();
	while (!ADC1.SR.EOC);
	estop_finish_sample();

	// Enable ADC completion interrupts
	ADC1.CR1.EOCIE = 1; // Enable interrupt on end of conversion
	NVIC_ISER[18 / 32] = 1 << (18 % 32); // SETENA18 = 1; enable ADC interrupt

	// Set up timer 7 to overflow every 10 milliseconds for sampling the emergency stop
	// Timer 7 input is 72 MHz from the APB
	// Need to count to 720,000 for each overflow
	// Set prescaler to 1,000, auto-reload to 720
	rcc_enable(APB1, TIM7);
	rcc_reset(APB1, TIM7);
	{
		TIM_basic_CR1_t tmp = {
			.ARPE = 0, // Auto-reload register is not buffered.
			.OPM = 0, // Counter counts forever.
			.URS = 1, // Only overflow generates and interrupt.
			.UDIS = 0, // Updates to control registers are allowed.
			.CEN = 0, // Counter is not counting right now.
		};
		TIM7.CR1 = tmp;
	}
	{
		TIM_basic_DIER_t tmp = {
			.UIE = 1, // Update interrupt enabled.
		};
		TIM7.DIER = tmp;
	}
	TIM7.PSC = 999;
	TIM7.ARR = 719;
	TIM7.CNT = 0;
	TIM7.CR1.CEN = 1; // Enable counter.
	NVIC_ISER[55 / 32] = 1 << (55 % 32); // SETENA55 = 1; enable timer 7 interrupt.
}

estop_t estop_read(void) {
	return value;
}

void estop_set_change_callback(estop_change_callback_t cb) {
	change_cb = cb;
}

