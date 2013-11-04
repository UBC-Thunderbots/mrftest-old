#include "buzzer.h"
#include <rcc.h>
#include <registers/nvic.h>
#include <registers/timer.h>

void timer5_interrupt_vector(void) {
	// The interrupt occurs when the buzzer should turn off.
	buzzer_stop();
	{
		TIM2_5_SR_t tmp = { 0 };
		TIM5.SR = tmp; // Clear interrupt flag
	}
}

void buzzer_init(void) {
	// Power up the modules
	rcc_enable(APB1, TIM2);
	rcc_reset(APB1, TIM2);
	rcc_enable(APB1, TIM5);
	rcc_reset(APB1, TIM5);

	// Configure timer 2 to drive the buzzer itself in PWM mode
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
		TIM2_5_SMCR_t tmp = { 0 }; // No external triggers or slave synchronization.
		TIM2.SMCR = tmp;
	}
	{
		TIM2_5_CCMR2_t tmp = {
			.O = {
				.OC3CE = 0, // Do not clear compare output 3 on external trigger.
				.OC3M = 4, // Compare output 3 forced low.
				.CC3S = 0, // CC3 channel configured as output.
			},
		};
		TIM2.CCMR2 = tmp;
	}
	{
		TIM2_5_CCER_t tmp = {
			.CC3NP = 0, // CC3 is an output, so this must be clear.
			.CC3P = 0, // CC3 is active high.
			.CC3E = 1, // CC3 signal is routed to pin.
		};
		TIM2.CCER = tmp;
	}
	TIM2.CNT = 0; // Clear counter
	TIM2.PSC = 0; // Set prescale 1:1
	TIM2.ARR = 36000000 * 2 / 2048; // Set frequency
	TIM2.CCR3 = 36000000 * 2 / 2048 / 2; // Set duty cycle
	TIM2.CR1.CEN = 1; // Counter enabled

	// Configure timer 5 to count down half-milliseconds remaining on the current buzzer sounding
	{
		TIM2_5_CR1_t tmp = {
			.CKD = 0, // Timer runs at full clock frequency.
			.ARPE = 0, // Auto-reload register is not buffered.
			.CMS = 0, // Counter counts in one direction.
			.DIR = 1, // Counter counts down.
			.OPM = 1, // Counter counts once and then stops.
			.URS = 1, // Counter overflow generates an interrupt; other activities do not.
			.UDIS = 0, // Updates to control registers are allowed.
			.CEN = 0, // Counter is not counting right now.
		};
		TIM5.CR1 = tmp;
	}
	{
		TIM2_5_SMCR_t tmp = { 0 }; // No external triggers or slave synchronization.
		TIM5.SMCR = tmp;
	}
	{
		TIM2_5_DIER_t tmp = {
			.UIE = 1, // Enable interrupt on timer update
		};
		TIM5.DIER = tmp; // Enable interrupt on timer update
	}
	TIM5.CNT = 0; // Clear timer
	TIM5.PSC = 35999; // Set prescale 1:36,000, yielding two ticks per millisecond
	TIM5.ARR = 1; // An auto-reload value of zero does not work; a value of 1, however, makes sure TIM5_CNT is very small when the counter stops, so it is less than future requests
	TIM5.CR1.CEN = 1; // Enable counter for one tick after which an interrupt will be delivered (not doing this appears to break the first enablement of the timer by instantly delivering an interrupt for no apparent reason)
	NVIC_ISER[50 / 32] = 1 << (50 % 32); // SETENA50 = 1; enable timer 5 interrupt
}

void buzzer_start(unsigned long millis) {
	if (TIM5.CNT < millis * 2UL) {
		TIM5.CNT = millis * 2UL;
		TIM5.CR1.CEN = 1; // Enable counter, in case it was disabled
	}
	{
		TIM2_5_CCMR2_t tmp = TIM2.CCMR2;
		tmp.O.OC3M = 6; // Compare output 3 PWM active when count < duty cycle.
		TIM2.CCMR2 = tmp;
	}
}

void buzzer_stop(void) {
	{
		TIM2_5_CCMR2_t tmp = TIM2.CCMR2;
		tmp.O.OC3M = 4; // Compare output 3 forced low.
		TIM2.CCMR2 = tmp;
	}
	{
		TIM2_5_SR_t tmp = { 0 }; // Clear interrupt flag.
		TIM5.SR = tmp;
	}
}

