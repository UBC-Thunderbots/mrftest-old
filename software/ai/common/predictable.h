#ifndef AI_COMMON_PREDICTABLE_H
#define AI_COMMON_PREDICTABLE_H

#include "geom/angle.h"
#include "geom/point.h"

namespace AI {
	/**
	 * An object whose velocity and future position can be predicted based on a collection of prior positions.
	 */
	class Predictable {
		public:
			/**
			 * \brief Gets the predicted position of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the predicted position.
			 */
			virtual Point position(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * \brief Gets the predicted velocity of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the predicted velocity.
			 */
			virtual Point velocity(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * \brief Gets the standard deviation of the predicted position of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the standard deviation of the predicted position.
			 */
			virtual Point position_stdev(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * \brief Gets the standard deviation of the predicted velocity of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the standard deviation of the predicted velocity.
			 */
			virtual Point velocity_stdev(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;
	};

	/**
	 * An object whose angular velocity and future orientation can be predicted in addition to position.
	 */
	class OrientationPredictable : public Predictable {
		public:
			/**
			 * \brief Gets the predicted orientation of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the predicted orientation.
			 */
			virtual Angle orientation(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * \brief Gets the predicted angular velocity of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the predicted angular velocity.
			 */
			virtual Angle avelocity(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * \brief Gets the standard deviation of the predicted orientation of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the standard deviation of the predicted orientation.
			 */
			virtual Angle orientation_stdev(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

			/**
			 * \brief Gets the standard deviation of the predicted angular velocity of the object.
			 *
			 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time.
			 *
			 * \return the standard deviation of the predicted angular velocity.
			 */
			virtual Angle avelocity_stdev(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;
	};
}

#endif

