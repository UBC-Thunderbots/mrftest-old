#include "buzzer.h"
#include "registers.h"

void buzzer_init(void) {
	// Configure timer 2 for the buzzer
	TIM2_CR1 = (TIM2_CR1 & 0b1111110000000000) // Reserved
		| (0 << 8) // CKD = 0; no division between internal clock and digital filter sampling clock
		| (0 << 7) // ARPE = 0; no buffering on auto-reload register
		| (0 << 5) // CMS = 0; edge-aligned counting mode
		| (0 << 4) // DIR = 0; counter counts up
		| (0 << 3) // OPM = 0; counter does not stop at an update event
		| (0 << 2) // URS = 0; overflow, UG bit, and slave mode updates all cause interrupts/DMA
		| (0 << 1) // UDIS = 0; do not disable normal scheduled register updates
		| (0 << 0); // CEN = 0; counter disabled for now
	TIM2_CR2 = (TIM2_CR2 & 0b1111111100000111) // Reserved
		| (0 << 7) // TI1S = 0; TIM1_CH1 input comes from TI1 input
		| (0 << 4); // MMS = 0; UG bit becomes trigger output to slave timers
	TIM2_SMCR = (TIM2_SMCR & 0b0000000000001000) // Reserved
		| (0 << 0); // SMS = 0; slave mode disabled
	TIM2_CCMR2 = (0 << 7) // OC3CE = 0; do not clear output on external trigger
		| (0b100 << 4) // OC3M = 100; output forced low
		| (0 << 3) // OC3PE = 0; no buffering on CCR
		| (0 << 2) // OC3FE = 0; no acceleration of trigger input effects
		| (0 << 0); // CC3S = 0; channel 3 configured as an output
	TIM2_CCER = (TIM2_CCER & 0b0100010001000100) // Reserved
		| (0 << 11) // CC3NP = 0; must be cleared for output channel
		| (0 << 9) // CC3P = 0; output is active high
		| (1 << 8); // CC3E = 1; output is sent to pin
	TIM2_CNT = 0; // Clear counter
	TIM2_PSC = 0; // Set prescale 1:1
	TIM2_ARR = 36000000 * 2 / 2048; // Set frequency
	TIM2_CCR3 = 36000000 * 2 / 2048 / 2; // Set duty cycle
	TIM2_CR1 |= 1 << 0; // CEN = 1; counter enabled
}

void buzzer_set(bool ctl) {
	if (ctl) {
		TIM2_CCMR2 = (0 << 7) // OC3CE = 0; do not clear output on external trigger
			| (0b110 << 4) // OC3M = 110; PWM mode active-high
			| (0 << 3) // OC3PE = 0; no buffering on CCR
			| (0 << 2) // OC3FE = 0; no acceleration of trigger input effects
			| (0 << 0); // CC3S = 0; channel 3 configured as an output
	} else {
		TIM2_CCMR2 = (0 << 7) // OC3CE = 0; do not clear output on external trigger
			| (0b100 << 4) // OC3M = 100; output forced low
			| (0 << 3) // OC3PE = 0; no buffering on CCR
			| (0 << 2) // OC3FE = 0; no acceleration of trigger input effects
			| (0 << 0); // CC3S = 0; channel 3 configured as an output
	}
}

