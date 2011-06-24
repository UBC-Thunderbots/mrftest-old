#ifndef AI_UTIL
#define AI_UTIL

#include "geom/point.h"

namespace AI {
	namespace Util {
		/**
		 * Computes where to chase the ball given the location of a player.
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
		bool grab_ball_dest(Point ball_pos, Point ball_vel, Point player_pos, Point& dest);
	}
}

#endif

