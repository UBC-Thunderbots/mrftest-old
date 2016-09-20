#ifndef AI_BACKEND_BALL_H
#define AI_BACKEND_BALL_H

#include "ai/common/time.h"
#include "geom/predictor.h"
#include "geom/point.h"
#include "uicomponents/visualizer.h"

namespace AI {
	namespace BE {
		/**
		 * \brief The ball, as exposed by the backend
		 */
		class Ball final : public Visualizable::Ball {
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
				void add_field_data(Point pos, AI::Timestamp ts);

				/**
				 * \brief Locks the current time for the predictors
				 *
				 * \param[in] now the current time
				 */
				void lock_time(AI::Timestamp now);

				/**
				 * \brief Returns the most recent locked timestamp.
				 *
				 * \return the lock timestamp.
				 */
				AI::Timestamp lock_time() const;

				Point position() const override;
				Point position(double delta) const;
				Point velocity() const override;
				Point velocity(double delta) const;
				Point position_stdev(double delta) const;
				Point velocity_stdev(double delta) const;
				bool highlight() const override;
				Visualizable::Colour highlight_colour() const override;

			private:
				Predictor2 pred;
				Point position_cached, velocity_cached;

				void update_caches();
		};
	}
}



inline Point AI::BE::Ball::position() const {
	return position_cached;
}

inline Point AI::BE::Ball::velocity() const {
	return velocity_cached;
}

#endif
