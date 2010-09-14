#ifndef AI_BALL_FILTER_BALL_FILTER_H
#define AI_BALL_FILTER_BALL_FILTER_H

#include "ai/ball_filter/world.h"
#include "geom/point.h"
#include "util/registerable.h"
#include <utility>
#include <vector>

namespace AI {
	namespace BF {
		/**
		 * An object capable of examining incoming ball information data and filtering it to determine which detections are real and which are spurious.
		 */
		class BallFilter : public Registerable<BallFilter> {
			public:
				/**
				 * Performs a filtering operation.
				 *
				 * \param[in] balls the set of balls detected by the two cameras, along with their associated confidence levels.
				 *
				 * \param[in] world the world in which to filter.
				 *
				 * \return the correct position of the ball.
				 */
				virtual Point filter(const std::vector<std::pair<double, Point> > &balls, AI::BF::W::World &world) = 0;

			protected:
				/**
				 * Constructs a new BallFilter.
				 *
				 * \param[in] name the name of the filter.
				 */
				BallFilter(const Glib::ustring &name) : Registerable<BallFilter>(name) {
				}
		};
	}
}

#endif

