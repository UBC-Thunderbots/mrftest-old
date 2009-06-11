#include <avr/io.h>
#include <avr/interrupt.h>

#include "adcpins.h"

volatile int16_t adc_results[8];
static uint8_t current_pin;

ISR(ADC_vect, ISR_BLOCK) {
	adc_results[current_pin] = ADC;
	current_pin = (current_pin + 1) % 8;
	ADMUX = (ADMUX & ~(_BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0))) | (current_pin << MUX0);
	ADCSRA |= _BV(ADSC);
}

void adc_init(void) {
	DDRF = 0;
	PORTF = 0;
	ADMUX = _BV(REFS0);
	ADCSR = _BV(ADEN) | _BV(ADSC) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
}

