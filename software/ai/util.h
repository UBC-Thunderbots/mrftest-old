#ifndef AI_UTIL
#define AI_UTIL

#include "geom/point.h"

namespace AI {
	namespace Util {
		/**
		 * \brief The distance robots need to be away from the ball when the game is stopped.
		 */
		constexpr double BALL_STOP_DIST = 0.5;

		/**
		 * Computes the best location to grab the ball,
		 * minimizing the time required.
		 *
		 * \param[in] ball_pos ball position
		 *
		 * \param[in] ball_vel ball velocity
		 *
		 * \param[in] player_pos player position
		 *
		 * \param[out] dest the location to chase the ball.
		 *
		 * \return true if the output is valid.
		 */
		bool calc_fastest_grab_ball_dest(Point ball_pos, Point ball_vel, Point player_pos, Point &dest);
	}
}

#endif

