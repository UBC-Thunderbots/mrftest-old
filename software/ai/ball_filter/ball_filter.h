#ifndef AI_BALL_FILTER_BALL_FILTER_H
#define AI_BALL_FILTER_BALL_FILTER_H

#include "geom/point.h"
#include "util/registerable.h"
#include <utility>
#include <vector>
#include "ai/world/team.h"

/**
 * An object capable of examining incoming ball information data and filtering
 * it to determine which detections are real and which are spurious.
 */
class BallFilter : public Registerable<BallFilter> {
	public:
		/**
		 * Performs a filtering operation.
		 *
		 * \param[in] balls the set of balls detected by the two cameras, along
		 * with their associated confidence levels.
		 *
		 * \return the correct position of the ball.
		 */
		virtual Point filter(const std::vector<std::pair<double, Point> > &balls, FriendlyTeam &friendly, EnemyTeam &enemy) = 0;

	protected:
		/**
		 * Constructs a new BallFilter.
		 *
		 * \param[in] name the name of the filter.
		 */
		BallFilter(const Glib::ustring &name) : Registerable<BallFilter>(name) {
		}
};

#endif

