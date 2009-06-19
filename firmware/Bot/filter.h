#ifndef FILTER_H
#define FILTER_H

/*
 * An arbitrary filter or controller.
 */
struct filter {
	const double *a;
	const double *b;
	double delayed[2];
};

/*
 * Initializes the filter with coefficients.
 */
void filter_init(struct filter *f, const double *a, const double *b);

/*
 * Initializes the filter with coefficients and also preloaded initial value.
 */
void filter_init2(struct filter *f, const double *a, const double *b, double init);

/*
 * Clears the delayed values in the filter.
 */
void filter_clear(struct filter *f);

/*
 * Adds a new value to the filter and produces the filter's output.
 */
double filter_process(struct filter *f, double input);

#endif

