#include "util/predictor.h"
#include "util/time.h"
#include <cstdlib>

Predictor::Predictor(bool angle, double measure_std, double accel_std) : filter(angle, measure_std, accel_std), zero_value(0, 0) {
	// Record current time.
	timespec_now(lock_timestamp);
}

std::pair<double, double> Predictor::value(double delta, unsigned int deriv, bool ignore_cache) const {
	if (-1e-9 < delta && delta < 1e-9 && deriv == 0 && !ignore_cache) {
		return zero_value;
	} else {
		Matrix guess, covariance;
		filter.predict(timespec_add(double_to_timespec(delta), lock_timestamp), guess, covariance);
		if (deriv == 0) {
			return std::make_pair(guess(0, 0), covariance(0, 0));
		} else if (deriv == 1) {
			return std::make_pair(guess(1, 0), covariance(1, 1));
		} else {
			std::abort();
		}
	}
}

void Predictor::lock_time(const timespec &ts) {
	lock_timestamp = ts;
	zero_value = value(0, 0, true);
}

void Predictor::add_datum(double value, const timespec &ts) {
	filter.update(value, ts);
}

void Predictor::clear() {
}

