#include "util/predictor.h"
#include "util/time.h"

#warning TODO: access to control noise: as if our robot command and relative robot positions

Predictor::Predictor(bool angle) : filter(angle), zero_value(0) {
	// Record current time.
	timespec_now(lock_timestamp);
}

double Predictor::value(double delta, unsigned int deriv) const {
	if (-1e-9 < delta && delta < 1e-9 && deriv == 0) {
		return zero_value;
	} else {
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
}

void Predictor::lock_time(const timespec &ts) {
	lock_timestamp = ts;
	zero_value = value(0);
}

void Predictor::add_datum(double value, const timespec &ts) {
	filter.update(value, ts);
}

void Predictor::clear() {
}

