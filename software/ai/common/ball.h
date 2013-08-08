#ifndef AI_COMMON_BALL_H
#define AI_COMMON_BALL_H

#include "geom/point.h"

namespace AI {
	namespace Common {
		/**
		 * \brief The common functions available on the ball in all layers.
		 */
		class Ball {
			public:
				/**
				 * \brief The approximate radius of the ball.
				 */
				static constexpr double RADIUS = 0.0215;

				/**
				 * \brief Gets the predicted position of the object.
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the predicted position
				 */
				virtual Point position(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

				/**
				 * \brief Gets the predicted velocity of the object.
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the predicted velocity
				 */
				virtual Point velocity(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

				/**
				 * \brief Gets the standard deviation of the predicted position of the object.
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the standard deviation of the predicted position
				 */
				virtual Point position_stdev(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;

				/**
				 * \brief Gets the standard deviation of the predicted velocity of the object.
				 *
				 * \param[in] delta the number of seconds forward or backward to predict, relative to the current time
				 *
				 * \return the standard deviation of the predicted velocity
				 */
				virtual Point velocity_stdev(double delta = 0.0) const __attribute__((warn_unused_result)) = 0;
		};
	}
}

#endif

