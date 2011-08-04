#include "util/predictor.h"
#include "geom/angle.h"
#include "util/time.h"
#include <cstdlib>

namespace {
	template<typename T> double value_to_double(T x);

	template<> double value_to_double<double>(double x) {
		return x;
	}

	template<> double value_to_double<Angle>(Angle x) {
		return x.to_radians();
	}

	template<typename T> T double_to_value(double x);

	template<> double double_to_value<double>(double x) {
		return x;
	}

	template<> Angle double_to_value<Angle>(double x) {
		return Angle::of_radians(x);
	}
}

template<> Predictor<double>::Predictor(double measure_std, double accel_std) : filter(false, measure_std, accel_std), zero_value(0, 0) {
	// Record current time.
	timespec_now(lock_timestamp);
}

template<> Predictor<Angle>::Predictor(Angle measure_std, Angle accel_std) : filter(true, measure_std.to_radians(), accel_std.to_radians()), zero_value(Angle::ZERO, Angle::ZERO) {
	// Record current time.
	timespec_now(lock_timestamp);
}

template<typename T> std::pair<T, T> Predictor<T>::value(double delta, unsigned int deriv, bool ignore_cache) const {
	if (-1e-9 < delta && delta < 1e-9 && !ignore_cache) {
		if (deriv == 0) {
			return zero_value;
		} else if (deriv == 1) {
			return zero_first_deriv;
		} else {
			std::abort();
		}
	} else {
		Matrix guess, covariance;
		filter.predict(timespec_add(double_to_timespec(delta), lock_timestamp), guess, covariance);
		if (deriv == 0) {
			return std::make_pair(double_to_value<T>(guess(0, 0)), double_to_value<T>(std::sqrt(covariance(0, 0))));
		} else if (deriv == 1) {
			return std::make_pair(double_to_value<T>(guess(1, 0)), double_to_value<T>(std::sqrt(covariance(1, 1))));
		} else {
			std::abort();
		}
	}
}

template<typename T> void Predictor<T>::lock_time(const timespec &ts) {
	lock_timestamp = ts;
	zero_value = value(0, 0, true);
	zero_first_deriv = value(0, 1, true);
}

template<typename T> void Predictor<T>::add_datum(T value, const timespec &ts) {
	filter.update(value_to_double(value), ts);
}

template<typename T> void Predictor<T>::clear() {
}

// Instantiate templates.
template class Predictor<double>;
template class Predictor<Angle>;

