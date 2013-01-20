#include "estop.h"
#include "rcc.h"
#include "registers.h"
#include "sleep.h"

static estop_t value = ESTOP_BROKEN;
static void (*change_cb)(void) = 0;

static void estop_start_sample(void) {
	ADC1_CR2 |= 1 << 30; // SWSTART = 1; start conversion
}

static void estop_finish_sample(void) {
	uint32_t reading = ADC1_DR;
	// EStop open produces nominal 1.086 V
	// EStop closed produces nominal 2.242 V
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
	TIM7_SR = 0; // UIF = 1; clear interrupt flag
	estop_start_sample();
}

void adc_interrupt_vector(void) {
	estop_t old = value;
	estop_finish_sample();
	if (value != old && change_cb) {
		change_cb();
	}
}

void estop_init(void) {
	// Send power to the switch
	GPIOB_BSRR = GPIO_BS(1);

	// Configure the ADC
	rcc_enable(APB2, 8);
	ADC_CSR = 0;
	ADC1_CR1 = 0;
	ADC1_CR2 = 1 << 0; // ADON = 1; enable ADC
	sleep_1us(3);
	ADC1_SMPR2 = 1 << 27; // SMP9 = 1; set sample time to 15 cycles
	ADC1_SQR1 = 0; // L = 0; conversion length 1
	ADC1_SQR3 = 9; // SQ1 = 9; first conversion is channel 9

	// Take one sample now
	estop_start_sample();
	while (!(ADC1_SR & (1 << 1) /* EOC */));
	estop_finish_sample();

	// Enable ADC completion interrupts
	ADC1_CR1 |= 1 << 5; // EOCIE = 1; enable interrupt on end of conversion
	NVIC_ISER[18 / 32] = 1 << (18 % 32); // SETENA18 = 1; enable ADC interrupt

	// Set up timer 7 to overflow every 10 milliseconds for sampling the emergency stop
	// Timer 7 input is 72 MHz from the APB
	// Need to count to 720,000 for each overflow
	// Set prescaler to 1,000, auto-reload to 720
	rcc_enable(APB1, 5);
	TIM7_CR1 = (TIM7_CR1 & 0b1111111101110000) // Reserved
		| (0 << 7) // ARPE = 0; ARR not buffered
		| (0 << 3) // OPM = 0; run continuously
		| (1 << 2) // URS = 1; only overflow generates an interrupt
		| (0 << 1) // UDIS = 0; updates not disabled
		| (0 << 0); // CEN = 0; counter not presently enabled
	TIM7_DIER = 1 << 0; // UIE = 1; update interrupt enabled
	TIM7_PSC = 999;
	TIM7_ARR = 719;
	TIM7_CNT = 0;
	TIM7_CR1 |= 1 << 0; // CEN = 1; enable counter
	NVIC_ISER[55 / 32] = 1 << (55 % 32); // SETENA55 = 1; enable timer 7 interrupt
}

estop_t estop_read(void) {
	return value;
}

void estop_set_change_callback(void (*cb)(void)) {
	change_cb = cb;
}

