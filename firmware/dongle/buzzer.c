#include "buzzer.h"
#include "rcc.h"
#include "registers.h"

void timer5_interrupt_vector(void) {
	// The interrupt occurs when the buzzer should turn off.
	TIM2_CCMR2 = (TIM2_CCMR2 & ~OC3M_MSK) | OC3M(4); // OC3M = 0b100; output forced low
	TIM5_SR = 0; // Clear interrupt flag
}

void buzzer_init(void) {
	// Power up the modules
	rcc_enable_multi(APB1, 0b1001); // Enable timer 5 and 2

	// Configure timer 2 to drive the buzzer itself in PWM mode
	TIM2_CR1 = 0 // Auto reload not buffered, counter counts up, continuously (not just for one pulse), updates not disabled, counter disabled for now
		| CMS(0); // Counter counts in one direction only
	TIM2_SMCR = 0; // Slave mode disabled
	TIM2_CCMR2 = 0 // Do not clear output on external trigger, no buffering on CCR, do not enable compare output on trigger input
		| OC3M(4) // OC3M = 0b100; compare output 3 forced low
		| CC3S(0); // CC3S = 0b00; CC3 configured as output
	TIM2_CCER = 0 // Compare output 3 is active high
		| CC3E; // Compare output 3 is enabled to I/O pin via alternate function map
	TIM2_CNT = 0; // Clear counter
	TIM2_PSC = 0; // Set prescale 1:1
	TIM2_ARR = 36000000 * 2 / 2048; // Set frequency
	TIM2_CCR3 = 36000000 * 2 / 2048 / 2; // Set duty cycle
	TIM2_CR1 |= CEN; // Counter enabled

	// Configure timer 5 to count down half-milliseconds remaining on the current buzzer sounding
	TIM5_CR1 = 0 // Auto reload not buffered, in one direction, updates not disabled, counter disabled for now
		| DIR // Counter counts down
		| OPM // Counter counts once, not continuously
		| URS; // Only underflow generates an interrupt
	TIM2_SMCR = 0; // Slave mode disabled
	TIM5_DIER = UIE; // Enable interrupt on timer update
	TIM5_CNT = 0; // Clear timer
	TIM5_PSC = 35999; // Set prescale 1:36,000, yielding two ticks per millisecond
	TIM5_ARR = 0xFFFFFFFF; // Set auto-reload to “infinity” because we don't really want auto-reload at all
	TIM5_CR1 |= CEN; // Enable counter for one tick after which an interrupt will be delivered (not doing this appears to break the first enablement of the timer by instantly delivering an interrupt for no apparent reason)
	NVIC_ISER[50 / 32] = 1 << (50 % 32); // SETENA50 = 1; enable timer 5 interrupt
}

void buzzer_start(unsigned long millis) {
	if (TIM5_CNT < millis * 2UL || TIM5_CNT == 0xFFFFFFFF) {
		TIM5_CNT = millis * 2UL;
		TIM5_CR1 |= CEN; // Enable counter
	}
	TIM2_CCMR2 = (TIM2_CCMR2 & ~OC3M_MSK) | OC3M(6); // OC3M = 0b110; PWM mode active high
}

