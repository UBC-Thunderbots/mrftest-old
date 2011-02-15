#ifndef AI_NAVIGATOR_UTIL_H
#define AI_NAVIGATOR_UTIL_H

#include "ai/flags.h"
#include "ai/navigator/world.h"
#include "geom/point.h"
#include "geom/util.h"
#include "util/time.h"
#include <utility>
#include <vector>

namespace AI {
	namespace Nav {
		namespace Util {
			/**
			 * Finds where to go and when to get there in order to intercept the moving ball along the route to dst
			 */
			std::pair<Point, timespec> get_ramball_location(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player);

			// bool has_ramball_location(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player);


			/**
			 * returns true if the destination is valid
			 */
			bool valid_dst(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player);

			/**
			 * Returns true if the straight line path between cur & dst has a maximum level of rules violation
			 * exactly equal to the violation level of cur
			 */
			bool valid_path(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player);

			/**
			 * Returns true if the straight line path between cur & dst has a maximum level of rules violation
			 * exactly equal to the violation level of cur. This method allows for extra rules to be imposed via the "extra_flags"
			 * parameter
			 */
			bool valid_path(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, unsigned int extra_flags);

			/**
			 * returns a list of legal points circling the destination. These set of points may be valuable as a search space for a navigator
			 * it is not garuenteed that the returned vector is non-empty
			 *
			 * \param[in] dst the target destination
			 *
			 * \param[in] world the world for field information
			 *
			 * \param[in] player the player thats being checked
			 */
			std::vector<Point> get_destination_alternatives(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player);


			/**
			 * returns a list of legal destinations that are on the boundaries of obstacles such as the ball
			 * or enemy robots. These set of points may be valuable as a search space for a navigator
			 *
			 * \param[in] world the world for field information
			 *
			 * \param[in] player the player thats being checked
			 */
			std::vector<Point> get_obstacle_boundaries(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player);

			/**
			 * returns a list of legal destinations that are on the boundaries of obstacles such as the ball
			 * or enemy robots. These set of points may be valuable as a search space for a navigator
			 *
			 * \param[in] world the world for field information
			 *
			 * \param[in] player the player thats being checked
			 */
			std::vector<Point> get_obstacle_boundaries(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, unsigned int added_flags);

			/**
			 * returns the next timespec
			 *
			 * \param[in] now the time now
			 *
			 * \param[in] p1 first point
			 *
			 * \param[in] p2 second point
			 *
			 * \param[in] target_velocity the desired velocity we want when we get there
			 */
			timespec get_next_ts(timespec now, Point &p1, Point &p2, Point target_velocity);
		}
	}
}

#endif

