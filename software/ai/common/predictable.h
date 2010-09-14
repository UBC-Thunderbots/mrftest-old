#ifndef AI_COMMON_PREDICTABLE_H
#define AI_COMMON_PREDICTABLE_H

#include "geom/point.h"

namespace AI {
	/**
	 * An object whose velocity, acceleration, and future position can be predicted based on a collection of prior positions.
	 */
	class Predictable {
		public:
			/**
			 * Gets the predicted current position.
			 *
			 * \return the predicted position.
			 */
			virtual Point position() const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets a predicted future position.
			 *
			 * \return the predicted position \p delta_time in the future.
			 */
			virtual Point future_position(double delta_time) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted current velocity.
			 *
			 * \return the predicted linear velocity in metres per second.
			 */
			virtual Point est_velocity() const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted linear acceleration.
			 *
			 * \return the predicted linear acceleration in metres per second squared.
			 */
			virtual Point est_acceleration() const __attribute__((warn_unused_result)) = 0;
	};

	/**
	 * An object whose angular velocity, angular acceleration, and future orientation can be predicted in addition to position.
	 */
	class OrientationPredictable : public Predictable {
		public:
			/**
			 * Gets the predicted current orientation.
			 *
			 * \return the predicted orientation.
			 */
			virtual double orientation() const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets a predicted future orientation.
			 *
			 * \return the predicted orientation \p delta_time in the future.
			 */
			virtual double future_orientation(double delta_time) const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted current angular velocity.
			 *
			 * \return the predicted angular velocity in radians per second.
			 */
			virtual double est_avelocity() const __attribute__((warn_unused_result)) = 0;

			/**
			 * Gets the predicted angular acceleration.
			 *
			 * \return the predicted angular acceleration in radians per second squared.
			 */
			virtual double est_aacceleration() const __attribute__((warn_unused_result)) = 0;
	};
}

#endif

