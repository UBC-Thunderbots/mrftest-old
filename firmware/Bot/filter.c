#include "filter.h"

void filter_init(struct filter *f, const double *a, const double *b) {
	f->a = a;
	f->b = b;
	filter_clear(f);
}

void filter_init2(struct filter *f, const double *a, const double *b, double init) {
	f->a = a;
	f->b = b;
	init /= b[0] + b[1] + b[2];
	f->delayed[0] = init;
	f->delayed[1] = init;
}

void filter_clear(struct filter *f) {
	f->delayed[0] = 0;
	f->delayed[1] = 0;
}

double filter_process(struct filter *f, double input) {
	double output;

	input = input - f->a[1] * f->delayed[0] / f->a[0] - f->a[2] * f->delayed[1] / f->a[0];
	output = f->b[0] * input + f->b[1] * f->delayed[0] / f->a[0] + f->b[2] * f->delayed[1] / f->a[0];
	f->delayed[1] = f->delayed[0];
	f->delayed[0] = input;
	return output;
}

