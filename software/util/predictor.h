#ifndef UTIL_PREDICTOR_H
#define UTIL_PREDICTOR_H

#include "util/time.h"
#include "util/kalman/kalman.h"

/**
 * Accumulates data points over time and predicts past, current, and future values and derivatives.
 */
class Predictor {
	public:
		/**
		 * Constructs a new Predictor.
		 *
		 * \param[in] angle \c true to construct a Predictor suitable for operating on angles
		 * (for example, including anti-windup code), or \c false to construct a Predictor suitable for operating on linear quantities.
		 */
		Predictor(bool angle);

		/**
		 * Gets the predicted value some length of time into the future (or past).
		 *
		 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
		 *
		 * \param[in] deriv the derivative level to take (\c 0 for position, \c 1 for velocity, \c 2 for acceleration, etc.).
		 *
		 * \return the value.
		 */
		double value(double delta = 0.0, unsigned int deriv = 0) const __attribute__((warn_unused_result));

		/**
		 * Gets the predicted value at some fixed point in time.
		 *
		 * \param[in] ts the time at which to predict the value.
		 *
		 * \param[in] deriv the derivative level to take (\c 0 for position, \c 1 for velocity, \c 2 for acceleration, etc.).
		 *
		 * \return the value.
		 */
		double value(const timespec &ts, unsigned int deriv = 0) const __attribute__((warn_unused_result));

		/**
		 * Locks in a timestamp to consider as the current time.
		 *
		 * \param[in] ts the timestamp.
		 */
		void lock_time(const timespec &ts);

		/**
		 * Pushes a new sample into the prediction engine.
		 *
		 * \param[in] value the value to add.
		 *
		 * \param[in] ts the timestamp at which the value was sampled.
		 */
		void add_datum(double value, const timespec &ts);

		/**
		 * Clears the accumulated history of the predictor.
		 * This means that on the next addition of a new datum, the predictor will estimate zero for all derivatives.
		 * Until the addition of a new datum, the predictor will not change its output.
		 * This is useful when coordinate transformation changes lead to large step changes in value, such as when switching field ends.
		 */
		void clear();

	private:
		bool initialized;
		timespec lock_timestamp;
		Kalman filter;

		void update();
};

#endif

