#ifndef AI_COMMON_PREDICTABLE_H
#define AI_COMMON_PREDICTABLE_H

#include "geom/point.h"
#include "util/time.h"

namespace AI {
	/**
	 * An object whose velocity, acceleration, and future position can be predicted based on a collection of prior positions.
	 */
	class Predictable {
		public:
			/**
			 * Gets the predicted position of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the predicted position.
			 */
			virtual Point position(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted position of the object.
			 *
			 * \param[in] ts the time at which to predict the position.
			 *
			 * \return the position.
			 */
			virtual Point position(const timespec &ts) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted velocity of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the predicted velocity.
			 */
			virtual Point velocity(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted velocity of the object.
			 *
			 * \param[in] ts the time at which to predict the velocity.
			 *
			 * \return the velocity.
			 */
			virtual Point velocity(const timespec &ts) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted acceleration of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the predicted acceleration.
			 */
			virtual Point acceleration(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted acceleration of the object.
			 *
			 * \param[in] ts the time at which to predict the acceleration.
			 *
			 * \return the acceleration.
			 */
			virtual Point acceleration(const timespec &ts) const __attribute__((warn_unused_result)) = 0;
	};

	/**
	 * An object whose angular velocity, angular acceleration, and future orientation can be predicted in addition to position.
	 */
	class OrientationPredictable : public Predictable {
		public:
			/**
			 * Gets the predicted orientation of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the predicted orientation.
			 */
			virtual double orientation(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted orientation of the object.
			 *
			 * \param[in] ts the time at which to predict the orientation.
			 *
			 * \return the orientation.
			 */
			virtual double orientation(const timespec &ts) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted angular velocity of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the predicted angular velocity.
			 */
			virtual double avelocity(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted angular velocity of the object.
			 *
			 * \param[in] ts the time at which to predict the angular velocity.
			 *
			 * \return the angular velocity.
			 */
			virtual double avelocity(const timespec &ts) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted angular acceleration of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the predicted angular acceleration.
			 */
			virtual double aacceleration(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted angular acceleration of the object.
			 *
			 * \param[in] ts the time at which to predict the angular acceleration.
			 *
			 * \return the angular acceleration.
			 */
			virtual double aacceleration(const timespec &ts) const __attribute__((warn_unused_result)) = 0;
	};
}

#endif

