#include <avr/io.h>
#include <util/delay.h>

#include "constants.h"
#include "gyro.h"
#include "adcpins.h"

// The gyro's zero point.
static double zero;

// Whether a gyro is installed.
static uint8_t installed;

void gyro_init(void) {
	int accumulator = 0;
	uint8_t i;

	i = GYRO_ZERO_SAMPLES;
	do {
		adc_sample();
		accumulator += adc_read(ADCPIN_GYRO_DATA) - adc_read(ADCPIN_GYRO_VREF);
		if (adc_read(ADCPIN_GYRO_VREF) < 50) {
			installed = 0;
			return;
		}
	} while (--i);
	zero = (double) accumulator / GYRO_ZERO_SAMPLES;
	installed = 1;
}

double gyro_read(void) {
	if (installed)
		return GYRO_TO_RADS * (adc_read(ADCPIN_GYRO_DATA) - adc_read(ADCPIN_GYRO_VREF) - zero);
	else
		return 0;
}

uint8_t gyro_present(void) {
	return installed;
}

