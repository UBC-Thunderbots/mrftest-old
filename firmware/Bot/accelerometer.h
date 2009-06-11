#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <stdint.h>
#include "filter.h"

struct accelerometer {
	struct filter filter;
	uint8_t adc_pin;
	double radius, velocity, zero;
};

void accelerometer_init(struct accelerometer *a, const double *filter_a, const double *filter_b, uint8_t adc_pin, double radius);
void accelerometer_clear(struct accelerometer *a);
void accelerometer_set(struct accelerometer *a, double velocity);
double accelerometer_update(struct accelerometer *a, double vt);

#endif

