#ifndef AI_BACKEND_BALL_H
#define AI_BACKEND_BALL_H

#include "ai/common/ball.h"
#include "geom/predictor.h"
#include "geom/point.h"
#include "uicomponents/visualizer.h"
#include "util/time.h"

namespace AI {
	namespace BE {
		/**
		 * \brief The ball, as exposed by the backend
		 */
		class Ball : public AI::Common::Ball, public Visualizable::Ball {
			public:
				/**
				 * \brief Whether or not the ball should be highlighted in the visualizer
				 */
				bool should_highlight;

				/**
				 * \brief Constructs a new Ball
				 */
				explicit Ball();

				/**
				 * Updates the position of the ball using new field data
				 *
				 * \param[in] pos the new position of the ball, in unswapped field coordinates
				 *
				 * \param[in] ts the time at which the ball was in the given position
				 */
				void add_field_data(Point pos, timespec ts);

				/**
				 * \brief Locks the current time for the predictors
				 *
				 * \param[in] now the current time
				 */
				void lock_time(timespec now);

				/**
				 * \brief Returns the most recent locked timestamp.
				 *
				 * \return the lock timestamp.
				 */
				timespec lock_time() const;

				Point position(double delta = 0.0) const;
				Point velocity(double delta = 0.0) const;
				Point position_stdev(double delta = 0.0) const;
				Point velocity_stdev(double delta = 0.0) const;
				bool highlight() const;
				Visualizable::Colour highlight_colour() const;

			private:
				Predictor2 pred;
		};
	}
}

#endif

