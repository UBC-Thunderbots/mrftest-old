#ifndef GEOM_PREDICTOR_H
#define GEOM_PREDICTOR_H

#include "geom/point.h"
#include "geom/kalman/kalman.h"
#include "util/time.h"
#include <utility>

/**
 * \brief Accumulates data points over time and predicts past, current, and future values and derivatives.
 *
 * \tparam T the type of quantity to predict over.
 */
template<typename T> class Predictor {
	public:
		/**
		 * \brief The type of a timestamp used to record control inputs, measurements, and estimations.
		 */
		typedef std::chrono::steady_clock::time_point Timestamp;

		/**
		 * \brief The type of the difference to two timestamps.
		 */
		typedef std::chrono::steady_clock::duration Timediff;

		/**
		 * \brief Constructs a new Predictor.
		 *
		 * \param[in] measure_std the expected standard deviation of the measurement noise.
		 *
		 * \param[in] accel_std the standard deviation of noise equivalent to unknown object acceleration.
		 *
		 * \param[in] decay_time_constant rate of velocity decay.
		 */
		Predictor(T measure_std, T accel_std, Timediff decay_time_constant);

		/**
		 * \brief Gets the predicted value some length of time into the future (or past).
		 *
		 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
		 *
		 * \param[in] deriv the derivative level to take (\c 0 for position or \c 1 for velocity).
		 *
		 * \param[in] ignore_cache \c true to ignore the lookaside, or \c false to use it.
		 *
		 * \return the value and its standard deviation.
		 */
		std::pair<T, T> value(double delta, unsigned int deriv = 0, bool ignore_cache = false) const __attribute__((warn_unused_result));

		/**
		 * \brief Locks in a timestamp to consider as the current time.
		 *
		 * \param[in] ts the timestamp.
		 */
		void lock_time(Timestamp ts);

		/**
		 * \brief Returns the most recent locked timestamp.
		 *
		 * \return the lock timestamp.
		 */
		Timestamp lock_time() const;

		/**
		 * \brief Pushes a new measurement into the prediction engine.
		 *
		 * \param[in] value the value to add.
		 *
		 * \param[in] ts the timestamp at which the value was sampled.
		 */
		void add_measurement(T value, Timestamp ts);

		/**
		 * \brief Pushes a new control value into the prediction engine.
		 *
		 * \param[in] value the value to add.
		 *
		 * \param[in] ts the timestamp at which the control input will take effect.
		 */
		void add_control(T value, Timestamp ts);

		/**
		 * \brief Clears the accumulated history of the predictor.
		 *
		 * This means that on the next addition of a new datum, the predictor will estimate zero for all derivatives.
		 * Until the addition of a new datum, the predictor will not change its output.
		 * This is useful when coordinate transformation changes lead to large step changes in value, such as when switching field ends.
		 */
		void clear();

	private:
		Timestamp lock_timestamp;
		Kalman filter;
		std::pair<T, T> zero_value, zero_first_deriv;
};

/**
 * \brief Combines two Predictor objects to perform prediction of X and Y coordinates.
 */
class Predictor2 {
	public:
		/**
		 * \brief Constructs a new Predictor2.
		 *
		 * \param[in] measure_std the expected standard deviation of the measurement noise on each axis.
		 *
		 * \param[in] accel_std the standard deviation of noise equivalent to unknown object acceleration on each axis.
		 *
		 * \param[in] decay_time_constant rate of velocity decay.
		 */
		Predictor2(double measure_std, double accel_std, Predictor<double>::Timediff decay_time_constant);

		/**
		 * \brief Gets the predicted value some length of time into the future (or past).
		 *
		 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
		 *
		 * \param[in] deriv the derivative level to take (\c 0 for position or \c 1 for velocity).
		 *
		 * \param[in] ignore_cache \c true to ignore the lookaside, or \c false to use it.
		 *
		 * \return the value and its standard deviation.
		 */
		std::pair<Point, Point> value(double delta, unsigned int deriv = 0, bool ignore_cache = false) const __attribute__((warn_unused_result));

		/**
		 * \brief Locks in a timestamp to consider as the current time.
		 *
		 * \param[in] ts the timestamp.
		 */
		void lock_time(Predictor<double>::Timestamp ts);

		/**
		 * \brief Returns the most recent locked timestamp.
		 *
		 * \return the lock timestamp.
		 */
		Predictor<double>::Timestamp lock_time() const;

		/**
		 * \brief Pushes a new measurement into the prediction engine.
		 *
		 * \param[in] value the value to add.
		 *
		 * \param[in] ts the timestamp at which the value was sampled.
		 */
		void add_measurement(Point value, Predictor<double>::Timestamp ts);

		/**
		 * \brief Pushes a new control input into the prediction engine.
		 *
		 * \param[in] value the value to add.
		 *
		 * \param[in] ts the timestamp at which the control input will take effect.
		 */
		void add_control(Point value, Predictor<double>::Timestamp ts);

		/**
		 * \brief Clears the accumulated history of the predictor.
		 *
		 * This means that on the next addition of a new datum, the predictor will estimate zero for all derivatives.
		 * Until the addition of a new datum, the predictor will not change its output.
		 * This is useful when coordinate transformation changes lead to large step changes in value, such as when switching field ends.
		 */
		void clear();

	private:
		Predictor<double> x, y;
};

/**
 * \brief Combines three Predictor objects to perform prediction of X, Y, and theta coordinates.
 */
class Predictor3 {
	public:
		/**
		 * \brief Constructs a new Predictor2.
		 *
		 * \param[in] measure_std_linear the expected standard deviation of the measurement noise on each linear axis.
		 *
		 * \param[in] accel_std_linear the standard deviation of noise equivalent to unknown object acceleration on each linear axis.
		 *
		 * \param[in] decay_time_constant_linear rate of linear velocity decay.
		 *
		 * \param[in] measure_std_angular the expected standard deviation of the measurement noise for rotation.
		 *
		 * \param[in] accel_std_angular the standard deviation of noise equivalent to unknown object angular acceleration.
		 *
		 * \param[in] decay_time_constant_angular rate of angular velocity decay.
		 */
		Predictor3(double measure_std_linear, double accel_std_linear, Predictor<double>::Timediff decay_time_constant_linear, Angle measure_std_angular, Angle accel_std_angular, Predictor<double>::Timediff decay_time_constant_angular);

		/**
		 * \brief Gets the predicted value some length of time into the future (or past).
		 *
		 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
		 *
		 * \param[in] deriv the derivative level to take (\c 0 for position or \c 1 for velocity).
		 *
		 * \param[in] ignore_cache \c true to ignore the lookaside, or \c false to use it.
		 *
		 * \return the value and its standard deviation.
		 */
		std::pair<std::pair<Point, Angle>, std::pair<Point, Angle>> value(double delta, unsigned int deriv = 0, bool ignore_cache = false) const __attribute__((warn_unused_result));

		/**
		 * \brief Locks in a timestamp to consider as the current time.
		 *
		 * \param[in] ts the timestamp.
		 */
		void lock_time(Predictor<double>::Timestamp ts);

		/**
		 * \brief Returns the most recent locked timestamp.
		 *
		 * \return the lock timestamp.
		 */
		Predictor<double>::Timestamp lock_time() const;

		/**
		 * \brief Pushes a new measurement into the prediction engine.
		 *
		 * \param[in] linear_value the positional measurement to add.
		 *
		 * \param[in] orientation the orientation measurement to add.
		 *
		 * \param[in] ts the timestamp at which the value was sampled.
		 */
		void add_measurement(Point linear_value, Angle orientation, Predictor<double>::Timestamp ts);

		/**
		 * \brief Pushes a new control input into the prediction engine.
		 *
		 * \param[in] linear_value the linear velocity input to add.
		 *
		 * \param[in] angular_value the angular velocity input to add.
		 *
		 * \param[in] ts the timestamp at which the control input will take effect.
		 */
		void add_control(Point linear_value, Angle angular_value, Predictor<double>::Timestamp ts);

		/**
		 * \brief Clears the accumulated history of the predictor.
		 *
		 * This means that on the next addition of a new datum, the predictor will estimate zero for all derivatives.
		 * Until the addition of a new datum, the predictor will not change its output.
		 * This is useful when coordinate transformation changes lead to large step changes in value, such as when switching field ends.
		 */
		void clear();

	private:
		Predictor<double> x, y;
		Predictor<Angle> t;
};

#endif

