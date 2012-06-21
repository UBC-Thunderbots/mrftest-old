#ifndef AI_WORLD_BALL_H
#define AI_WORLD_BALL_H

#include "ai/backend/backend.h"
#include "proto/messages_robocup_ssl_detection.pb.h"
#include "util/noncopyable.h"
#include "util/predictor.h"
#include "util/time.h"
#include <cstdlib>
#include <sigc++/trackable.h>

namespace AI {
	namespace BE {
		namespace Hybrid {
			/**
			 * The ball.
			 */
			class Ball : public AI::BE::Ball, public sigc::trackable {
				public:
					/**
					 * Constructs a new Ball.
					 *
					 * \param[in] backend the backend the ball is part of.
					 */
					explicit Ball(AI::BE::Backend &backend);

					/**
					 * Updates the position of the ball using new data.
					 *
					 * \param[in] pos the new position of the ball, in unswapped field coordinates.
					 *
					 * \param[in] ts the time at which the ball was in the given position.
					 */
					void update(Point pos, timespec ts);

					/**
					 * Locks the current time for the predictors.
					 *
					 * \param[in] now the current time.
					 */
					void lock_time(timespec now);

					Point position(double delta = 0.0) const;
					Point velocity(double delta = 0.0) const;
					Point position_stdev(double delta = 0.0) const;
					Point velocity_stdev(double delta = 0.0) const;
					bool highlight() const;
					Visualizable::Colour highlight_colour() const;

				private:
					AI::BE::Backend &backend;
					Predictor2 pred;

					void on_defending_end_changed();
			};
		}
	}
}

#endif

