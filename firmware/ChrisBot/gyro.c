#include <avr/io.h>
#include <util/delay.h>

#include "constants.h"
#include "gyro.h"
#include "adcpins.h"

// The gyro's zero point.
static double zero;

void gyro_init(void) {
	int accumulator = 0;
	uint8_t i;

	i = GYRO_ZERO_SAMPLES;
	do {
		_delay_ms(5);
		accumulator += adc_read(ADCPIN_GYRO_DATA) - adc_read(ADCPIN_GYRO_VREF);
	} while (--i);
	zero = (double) accumulator / GYRO_ZERO_SAMPLES;
}

double gyro_read(void) {
	return adc_read(ADCPIN_GYRO_DATA) - adc_read(ADCPIN_GYRO_VREF) - zero;
}

