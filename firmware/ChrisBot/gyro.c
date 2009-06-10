#include <avr/io.h>
#include <util/delay.h>

#include "constants.h"
#include "gyro.h"
#include "adcpins.h"

// The gyro's zero point.
static double zero;

void init_gyro(void) {
	int accumulator = 0;
	uint8_t i;

	i = GYRO_ZERO_SAMPLES;
	do {
		_delay_ms(5);
		accumulator += adc_results[ADCPIN_GYRO_DATA] - adc_results[ADCPIN_GYRO_VREF];
	} while (--i);
	zero = (double) accumulator / GYRO_ZERO_SAMPLES;
}

double read_gyro(void) {
	return adc_results[ADCPIN_GYRO_DATA] - adc_results[ADCPIN_GYRO_VREF] - gyro_zero;
}

