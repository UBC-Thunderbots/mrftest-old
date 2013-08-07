#include "geom/predictor.h"
#include "geom/angle.h"
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

template<> Predictor<double>::Predictor(double measure_std, double accel_std, Timediff decay_time_constant) : filter(false, measure_std, accel_std, decay_time_constant), zero_value(0, 0) {
}

template<> Predictor<Angle>::Predictor(Angle measure_std, Angle accel_std, Timediff decay_time_constant) : filter(true, measure_std.to_radians(), accel_std.to_radians(), decay_time_constant), zero_value(Angle::zero(), Angle::zero()) {
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
		filter.predict(lock_timestamp + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(delta)), guess, covariance);
		if (deriv == 0) {
			return std::make_pair(double_to_value<T>(guess(0, 0)), double_to_value<T>(std::sqrt(covariance(0, 0))));
		} else if (deriv == 1) {
			return std::make_pair(double_to_value<T>(guess(1, 0)), double_to_value<T>(std::sqrt(covariance(1, 1))));
		} else {
			std::abort();
		}
	}
}

template<typename T> void Predictor<T>::lock_time(Timestamp ts) {
	lock_timestamp = ts;
	zero_value = value(0, 0, true);
	zero_first_deriv = value(0, 1, true);
}

template<typename T> typename Predictor<T>::Timestamp Predictor<T>::lock_time() const {
	return lock_timestamp;
}

template<typename T> void Predictor<T>::add_measurement(T value, Timestamp ts) {
	filter.update(value_to_double(value), ts);
}

template<typename T> void Predictor<T>::add_control(T value, Timestamp ts) {
	filter.add_control(value_to_double(value), ts);
}

template<typename T> void Predictor<T>::clear() {
}

// Instantiate templates.
template class Predictor<double>;
template class Predictor<Angle>;



Predictor2::Predictor2(double measure_std, double accel_std, Predictor<double>::Timediff decay_time_constant) :
		x(measure_std, accel_std, decay_time_constant),
		y(measure_std, accel_std, decay_time_constant) {
}

std::pair<Point, Point> Predictor2::value(double delta, unsigned int deriv, bool ignore_cache) const {
	const std::pair<double, double> &vx = x.value(delta, deriv, ignore_cache);
	const std::pair<double, double> &vy = y.value(delta, deriv, ignore_cache);
	return std::make_pair(Point(vx.first, vy.first), Point(vx.second, vy.second));
}

void Predictor2::lock_time(Predictor<double>::Timestamp ts) {
	x.lock_time(ts);
	y.lock_time(ts);
}

Predictor<double>::Timestamp Predictor2::lock_time() const {
	return x.lock_time();
}

void Predictor2::add_measurement(Point value, Predictor<double>::Timestamp ts) {
	x.add_measurement(value.x, ts);
	y.add_measurement(value.y, ts);
}

void Predictor2::add_control(Point value, Predictor<double>::Timestamp ts) {
	x.add_control(value.x, ts);
	y.add_control(value.y, ts);
}

void Predictor2::clear() {
	x.clear();
	y.clear();
}



Predictor3::Predictor3(double measure_std_linear, double accel_std_linear, Predictor<double>::Timediff decay_time_constant_linear, Angle measure_std_angular, Angle accel_std_angular, Predictor<double>::Timediff decay_time_constant_angular) :
		x(measure_std_linear, accel_std_linear, decay_time_constant_linear),
		y(measure_std_linear, accel_std_linear, decay_time_constant_linear),
		t(measure_std_angular, accel_std_angular, decay_time_constant_angular) {
}

std::pair<std::pair<Point, Angle>, std::pair<Point, Angle>> Predictor3::value(double delta, unsigned int deriv, bool ignore_cache) const {
	const std::pair<double, double> &vx = x.value(delta, deriv, ignore_cache);
	const std::pair<double, double> &vy = y.value(delta, deriv, ignore_cache);
	const std::pair<Angle, Angle> &vt = t.value(delta, deriv, ignore_cache);
	return std::make_pair(std::make_pair(Point(vx.first, vy.first), vt.first), std::make_pair(Point(vx.second, vy.second), vt.second));
}

void Predictor3::lock_time(Predictor<double>::Timestamp ts) {
	x.lock_time(ts);
	y.lock_time(ts);
	t.lock_time(ts);
}

Predictor<double>::Timestamp Predictor3::lock_time() const {
	return x.lock_time();
}

void Predictor3::add_measurement(Point position, Angle orientation, Predictor<double>::Timestamp ts) {
	x.add_measurement(position.x, ts);
	y.add_measurement(position.y, ts);
	t.add_measurement(orientation, ts);
}

void Predictor3::add_control(Point linear_value, Angle angular_value, Predictor<double>::Timestamp ts) {
	x.add_control(linear_value.x, ts);
	y.add_control(linear_value.y, ts);
	t.add_control(angular_value, ts);
}

void Predictor3::clear() {
	x.clear();
	y.clear();
	t.clear();
}

