#include <util/delay.h>

#include "constants.h"
#include "accelerometer.h"
#include "adcpins.h"
#include "debug.h"

void accelerometer_init(struct accelerometer *a, const double *filter_a, const double *filter_b, uint8_t adc_pin, double radius) {
	int32_t accumulator;
	uint16_t i;

	filter_init(&a->filter, filter_a, filter_b);
	a->adc_pin = adc_pin;
	a->radius = radius;

	debug_puts("Radius="); debug_putf(a->radius); debug_putc('\n');

	accumulator = 0;
	i = ACCELEROMETER_ZERO_SAMPLES;
	do {
		adc_sample();
		accumulator += (int32_t) adc_read(adc_pin);
	} while (--i);
	a->zero = (double) accumulator / ACCELEROMETER_ZERO_SAMPLES;
	debug_puts("Zero=");
	debug_putf(a->zero);
	debug_putc('\n');

	accelerometer_clear(a);
}

void accelerometer_clear(struct accelerometer *a) {
	a->velocity = 0;
	filter_clear(&a->filter);
}

void accelerometer_set(struct accelerometer *a, double velocity) {
	a->velocity = velocity;
}

double accelerometer_update(struct accelerometer *a, double vt) {
	double diff = (-ACCELEROMETER_TO_CM * ((double) adc_read(a->adc_pin) - a->zero) + vt * vt * a->radius) * LOOP_TIME / 1000.0;
	a->velocity += diff;
	return filter_process(&a->filter, a->velocity);
}

