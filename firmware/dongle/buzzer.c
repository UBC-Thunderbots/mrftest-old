#include "buzzer.h"
#include "registers.h"

void timer5_interrupt_vector(void) {
	// The interrupt occurs when the buzzer should turn off.
	TIM2_CCMR2 = (0 << 7) // OC3CE = 0; do not clear output on external trigger
		| (0b100 << 4) // OC3M = 100; output forced low
		| (0 << 3) // OC3PE = 0; no buffering on CCR
		| (0 << 2) // OC3FE = 0; no acceleration of trigger input effects
		| (0 << 0); // CC3S = 0; channel 3 configured as an output
	TIM5_SR &= ~(1 << 0);
}

void buzzer_init(void) {
	// Reset the module and enable the clock
	RCC_APB1RSTR |= (1 << 3) // TIM5RST = 1; reset timer 5
		| (1 << 0); // TIM2RST = 1; reset timer 2
	asm volatile("dsb");
	asm volatile("nop");
	RCC_APB1ENR |= (1 << 3) // TIM5EN = 1; enable clock to timer 5
		| (1 << 0); // TIM2EN = 1; enable clock to timer 2
	asm volatile("dsb");
	asm volatile("nop");
	RCC_APB1RSTR &= ~((1 << 3) // TIM5RST = 0; release timer 5 from reset
		| (1 << 0)); // TIM2RST = 0; release timer 2 from reset
	asm volatile("dsb");
	asm volatile("nop");

	// Configure timer 2 to drive the buzzer itself in PWM mode
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
		| (0 << 7) // TI1S = 0; TIM2_CH1 input comes from TI1 input
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

	// Configure timer 5 to count down half-milliseconds remaining on the current buzzer sounding
	TIM5_CR1 = (TIM5_CR1 & 0b1111110000000000) // Reserved
		| (0 << 8) // CKD = 0; no division between internal clock and digital filter sampling clock
		| (0 << 7) // ARPE = 0; no buffering on auto-reload register
		| (0 << 5) // CMS = 0; edge-aligned counting mode
		| (1 << 4) // DIR = 1; counter counts down
		| (1 << 3) // OPM = 1; counter stops at an update event
		| (1 << 2) // URS = 1; only underflow generates an interrupt
		| (0 << 1) // UDIS = 0; do not disable normal scheduled register updates
		| (0 << 0); // CEN = 0; counter disabled until needed
	TIM5_CR2 = (TIM5_CR2 & 0b1111111100000111) // Reserved
		| (0 << 7) // TI1S = 0; TIM5_CH1 comes from TI1 input
		| (0 << 4); // MMS = 0; UG bit becomes trigger output to slave timers
	TIM5_SMCR = (TIM5_SMCR & 0b0000000000001000) // Reserved
		| (0 << 0); // SMS = 0; slave mode disabled
	TIM5_DIER = (TIM5_DIER & 0b1010000010100001) // Reserved
		| (1 << 0); // UIE = 1; enable interrupt on timer update
	TIM5_CNT = 0; // Clear timer
	TIM5_PSC = 35999; // Set prescale 1:36,000, yielding two ticks per millisecond
	TIM5_ARR = 0xFFFFFFFF; // Set auto-reload to “infinity” because we don't really want auto-reload at all
	TIM5_CR1 |= 1 << 0; // CEN = 1; enable counter for one tick after which an interrupt will be delivered (not doing this appears to break the first enablement of the timer by instantly delivering an interrupt for no apparent reason)
	NVIC_ISER(50 / 32) = 1 << (50 % 32); // SETENA50 = 1; enable timer 5 interrupt
}

void buzzer_start(unsigned long millis) {
	if (TIM5_CNT < millis * 2UL || TIM5_CNT == 0xFFFFFFFF) {
		TIM5_CNT = millis * 2UL;
		TIM5_CR1 |= 1 << 0; // CEN = 1; enable counter
	}
	TIM2_CCMR2 = (0 << 7) // OC3CE = 0; do not clear output on external trigger
		| (0b110 << 4) // OC3M = 110; PWM mode active-high
		| (0 << 3) // OC3PE = 0; no buffering on CCR
		| (0 << 2) // OC3FE = 0; no acceleration of trigger input effects
		| (0 << 0); // CC3S = 0; channel 3 configured as an output
}

