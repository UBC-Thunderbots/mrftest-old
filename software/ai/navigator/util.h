#ifndef AI_NAVIGATOR_UTIL_H
#define AI_NAVIGATOR_UTIL_H

#include "ai/flags.h"
#include "ai/navigator/world.h"
#include "geom/point.h"
#include "geom/util.h"
#include <vector>

namespace AI {
	namespace Nav {
		namespace Util {
			/**
			 *
			 */
			bool valid_dst(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player);

			/**
			 *
			 */
			bool valid_path(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player);

			/**
			 * Checks to see if the point is valid.
			 * This includes flag checks and checks to see if the dest is on another robot.
			 *
			 * \param[in] dest the point we want to check
			 *
			 * \param[in] world the world for field information
			 *
			 * \param[in] player the player thats being checked
			 *
			 * \return \c true if destination is valid and \c false if not.
			 */
			bool check_dest_valid(Point dest, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player);
		}
	}
}

#endif

