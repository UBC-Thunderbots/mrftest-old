#include "util/predictor.h"
#include "util/time.h"

#warning TODO: access to control noise: as if our robot command and relative robot positions

Predictor::Predictor(bool angle) : filter(angle) {
	// Record current time.
	timespec_now(lock_timestamp);
}

double Predictor::value(double delta, unsigned int deriv) const {
	const Matrix &guess = filter.predict(timespec_add(double_to_timespec(delta), lock_timestamp));
	double v = 0;
	if (deriv == 0) {
		v = guess(0, 0);
	} else if (deriv == 1) {
		v = guess(1, 0);
	} else if (deriv == 2) {
		v = filter.get_control(timespec_add(double_to_timespec(delta), lock_timestamp));
	} else {
		v = 0.0;
	}

	return v;
}

void Predictor::lock_time(const timespec &ts) {
	lock_timestamp = ts;
}

void Predictor::add_datum(double value, const timespec &ts) {
	filter.update(value, ts);
}

void Predictor::clear() {
}

