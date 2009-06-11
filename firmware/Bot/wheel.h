#ifndef WHEEL_H
#define WHEEL_H

#include <stdint.h>
#include "filter.h"

struct wheel {
	struct filter rpm_filter;
	struct filter controller;
	uint8_t counter_oe_pin, motor_a_pin, motor_b_pin, motor_pwm_pin, cur_count, last_count;
	const double *scale_factors;
};

void wheel_init(struct wheel *w, uint8_t counter_oe_pin, uint8_t motor_a_pin, uint8_t motor_b_pin, uint8_t motor_pwm_pin, const double *scale_factors, const double *rpm_filter_a, const double *rpm_filter_b, const double *controller_a, const double *controller_b);
void wheel_clear(struct wheel *w);
void wheel_update_rpm(struct wheel *w);
void wheel_update_drive(struct wheel *w, double vx, double vy, double vt);

#endif

