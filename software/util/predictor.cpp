#include "util/predictor.h"
#include "leastsquares/leastsquares.h"
#include "util/time.h"
#include "util/timestep.h"
#include <iostream>
#include <cmath>

#warning TODO 1. accompensate for delay in real mode, and not to in simulator mode
#warning TODO 2. access to control noise: as if our robot command and relative robot positions
#warning TODO 3. accompensate for glitches when "clipping" angles

namespace {
	const int MAX_DEGREE = 3;
	const int NUM_OLD_POSITIONS = 30;
}

Predictor::Predictor(bool angle) : initialized(false), lock_delta(0.0), filter(angle) {
	// Record current time.
	timespec_now(last_datum_timestamp);
	lock_timestamp = last_datum_timestamp;
	filter.set_availability(true);
}

Predictor::~Predictor() {
}

double Predictor::value(double delta, unsigned int deriv) const {
	timespec total;
	timespec_add(double_to_timespec(delta), lock_timestamp, total);
	return value(total, deriv);
}

double Predictor::value(const timespec &ts, unsigned int deriv) const {
	double v = 0;
	matrix guess = filter.predict(timespec_to_double(ts));
	if (deriv == 0) {
		v = guess(0, 0);
	} else if (deriv == 1) {
		v = guess(1, 0);
	} else if (deriv == 2) {
		v = filter.get_control(timespec_to_double(ts));
	} else {
		v = 0.0;
	}

	return v;
}

void Predictor::lock_time(const timespec &ts) {
	lock_timestamp = ts;
	timespec diff;
	timespec_sub(ts, last_datum_timestamp, diff);
	lock_delta = timespec_to_double(diff);
}

void Predictor::add_datum(double value, const timespec &ts) {
	// If this is our first datum, do a clear instead.
	if (!initialized) {
		// Remember that we're initialized.
		initialized = true;

		// Mark the current time as the most recent datum stamp.
		last_datum_timestamp = ts;

		// Update the predictions.
		// update();
		// or use kalman filter
		filter.update(value, timespec_to_double(ts));

		// Don't do any of the rest.
		return;
	}

	// Compute delta time and update stamp.
	last_datum_timestamp = ts;

	filter.update(value, timespec_to_double(ts));
}

void Predictor::clear() {
	initialized = false;
}

void Predictor::update() {
}

