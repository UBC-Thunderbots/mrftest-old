#ifndef AI_BALL_FILTER_BALL_FILTER_H
#define AI_BALL_FILTER_BALL_FILTER_H

#include "geom/point.h"
#include "util/registerable.h"
#include <utility>
#include <vector>

/**
 * An object capable of examining incoming ball information data and filtering
 * it to determine which detections are real and which are spurious.
 */
class ball_filter : public registerable<ball_filter> {
	public:
		/**
		 * Performs a filtering operation.
		 *
		 * \param balls the set of balls detected by the two cameras, along with
		 * their associated confidence levels
		 *
		 * \return The correct position of the ball.
		 */
		virtual point filter(const std::vector<std::pair<point, double> > &balls) = 0;

	protected:
		/**
		 * Constructs a new ball_filter.
		 *
		 * \param name the name of the filter.
		 */
		ball_filter(const Glib::ustring &name) : registerable<ball_filter>(name) {
		}
};

#endif

