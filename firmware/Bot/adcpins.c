#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "constants.h"
#include "iopins.h"
#include "adcpins.h"

int16_t adc_results[8];

ISR(ADC_vect, ISR_BLOCK) {
	// Do nothing, but will allow waking from sleep.
}

void adc_init(void) {
	// Configure pins as inputs.
	iopin_configure_input(IOPIN_ADC0);
	iopin_configure_input(IOPIN_ADC1);
	iopin_configure_input(IOPIN_ADC2);
	iopin_configure_input(IOPIN_ADC3);
	iopin_configure_input(IOPIN_ADC4);
	iopin_configure_input(IOPIN_ADC5);
	iopin_configure_input(IOPIN_ADC6);
	iopin_configure_input(IOPIN_ADC7);

	// Turn off pull-up resistors.
	iopin_write(IOPIN_ADC0, 0);
	iopin_write(IOPIN_ADC1, 0);
	iopin_write(IOPIN_ADC2, 0);
	iopin_write(IOPIN_ADC3, 0);
	iopin_write(IOPIN_ADC4, 0);
	iopin_write(IOPIN_ADC5, 0);
	iopin_write(IOPIN_ADC6, 0);
	iopin_write(IOPIN_ADC7, 0);
}

void adc_sample(void) {
	uint8_t i;

	// Turn on the ADC.
	ADMUX = _BV(REFS0);
	ADCSR = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);

	for (i = 0; i < sizeof(adc_results) / sizeof(*adc_results); i++) {
		// Set the MUX to this channel.
		ADMUX = (ADMUX & ~(_BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1) | _BV(MUX0))) | (i << MUX0);
		// Wait a couple of microseconds for the channel to settle (due to higher-than-desired impedance).
		_delay_us(ADC_SETTLE_DELAY);
		// Start the conversion.
		ADCSR |= _BV(ADSC);
		// Go to sleep until conversion finished.
		sleep_enable();
		while (ADCSR & _BV(ADSC)) {
			cli();
			if (ADCSR & _BV(ADSC)) {
				sei();
				sleep_cpu();
			}
			sei();
		}
		sleep_disable();
		// Copy the conversion result into the output buffer.
		adc_results[i] = ADC;
	}

	// Turn off the ADC.
	ADCSR = 0;
}

