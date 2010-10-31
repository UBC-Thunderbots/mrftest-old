#ifndef AI_NAVIGATOR_UTIL_H
#define AI_NAVIGATOR_UTIL_H

#include <vector>
#include "geom/util.h"
#include "ai/flags.h"
#include "geom/point.h"
#include "ai/navigator/world.h"

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
		}
	}
}

#endif

