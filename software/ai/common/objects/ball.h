#ifndef AI_COMMON_OBJECTS_BALL_H
#define AI_COMMON_OBJECTS_BALL_H

#include "ai/backend/ball.h"
#include "geom/point.h"

namespace AI {
	namespace Common {
		class Ball;
	}
}

namespace AI {
	namespace Common {
		/**
		 * \brief The common functions available on the ball in all layers.
		 */
		class Ball final {
			public:
				/**
				 * \brief The approximate radius of the ball.
				 */
				static constexpr double RADIUS = 0.0215;

				/**
				 * \brief Constructs a new Ball.
				 *
				 * \param[in] impl the backend implementation
				 */
				explicit Ball(const AI::BE::Ball &impl);

				/**
				 * \brief Gets the predicted position of the object at the
				 * current time.
				 *
				 * \return the predicted position
				 */
				Point position() const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the predicted position of the object.
				 *
				 * \param[in] delta the number of seconds forward or backward
				 * to predict, relative to the current time
				 *
				 * \return the predicted position
				 */
				Point position(double delta) const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the predicted velocity of the object at the
				 * current time.
				 *
				 * \return the predicted velocity
				 */
				Point velocity() const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the predicted velocity of the object.
				 *
				 * \param[in] delta the number of seconds forward or backward
				 * to predict, relative to the current time
				 *
				 * \return the predicted velocity
				 */
				Point velocity(double delta) const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the standard deviation of the predicted position
				 * of the object.
				 *
				 * \param[in] delta the number of seconds forward or backward
				 * to predict, relative to the current time
				 *
				 * \return the standard deviation of the predicted position
				 */
				Point position_stdev(double delta = 0.0) const __attribute__((warn_unused_result));

				/**
				 * \brief Gets the standard deviation of the predicted velocity
				 * of the object.
				 *
				 * \param[in] delta the number of seconds forward or backward
				 * to predict, relative to the current time
				 *
				 * \return the standard deviation of the predicted velocity
				 */
				Point velocity_stdev(double delta = 0.0) const __attribute__((warn_unused_result));

			private:
				const AI::BE::Ball &impl;
		};
	}
}



inline AI::Common::Ball::Ball(const AI::BE::Ball &impl) : impl(impl) {
}

inline Point AI::Common::Ball::position() const {
	return impl.position();
}

inline Point AI::Common::Ball::position(double delta) const {
	return impl.position(delta);
}

inline Point AI::Common::Ball::velocity() const {
	return impl.velocity();
}

inline Point AI::Common::Ball::velocity(double delta) const {
	return impl.velocity(delta);
}

inline Point AI::Common::Ball::position_stdev(double delta) const {
	return impl.position_stdev(delta);
}

inline Point AI::Common::Ball::velocity_stdev(double delta) const {
	return impl.velocity_stdev(delta);
}

#endif
