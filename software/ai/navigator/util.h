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

