#ifndef ADCPINS_H
#define ADCPINS_H

#include <avr/interrupt.h>

/*
 * Initializes the ADC subsystem.
 */
void adc_init(void);

/*
 * Takes a set of ADC samples.
 */
void adc_sample(void);

/*
 * DO NOT USE THIS VARIABLE!
 */
extern int16_t adc_results[8];

/*
 * Reads a value from the ADC.
 */
#define adc_read(pin) \
	adc_results[pin]

#endif

