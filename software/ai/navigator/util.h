#ifndef AI_NAVIGATOR_UTIL_H
#define AI_NAVIGATOR_UTIL_H

#include "ai/navigator/world.h"
#include "geom/point.h"
#include "geom/util.h"
#include "ai/navigator/rrt_navigator.h"
#include <utility>
#include <vector>
#include <cairomm/context.h>
#include <cairomm/refptr.h>


namespace AI {
	namespace Nav {
		namespace Util {
			/**
			 * Estimates the time taken by a robot moving through each point in sequence.
			 */
			double estimate_action_duration(std::vector<std::pair<Point, Angle>> path_points);

			/**
			 * Finds where to go and when to get there in order to intercept the moving ball along the route to dst
			 */
			std::pair<Point, AI::Timestamp> get_ramball_location(Point dst, AI::Nav::W::World world, AI::Nav::W::Player player);

			// bool has_ramball_location(Point dst, AI::Nav::W::World world, AI::Nav::W::Player player);


			/**
			 * returns true if the destination is valid
			 */
			bool valid_dst(Point dst, AI::Nav::W::World world, AI::Nav::W::Player player);

			/**
			 * Returns true if the straight line path between cur & dst has a maximum level of rules violation
			 * exactly equal to the violation level of cur
			 */
			bool valid_path(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player);

			/**
			 * Returns true if the straight line path between cur & dst has a maximum level of rules violation
			 * exactly equal to the violation level of cur. This method allows for extra rules to be imposed via the "extra_flags"
			 * parameter
			 */
			bool valid_path(Point cur, Point dst, AI::Nav::W::World world, AI::Nav::W::Player player, unsigned int extra_flags);

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
			std::vector<Point> get_destination_alternatives(Point dst, AI::Nav::W::World world, AI::Nav::W::Player player);


			/**
			 * returns a list of legal destinations that are on the boundaries of obstacles such as the ball
			 * or enemy robots. These set of points may be valuable as a search space for a navigator
			 *
			 * \param[in] world the world for field information
			 *
			 * \param[in] player the player thats being checked
			 */
			std::vector<Point> get_obstacle_boundaries(AI::Nav::W::World world, AI::Nav::W::Player player);

			/**
			 * returns a list of legal destinations that are on the boundaries of obstacles such as the ball
			 * or enemy robots. These set of points may be valuable as a search space for a navigator
			 *
			 * \param[in] world the world for field information
			 *
			 * \param[in] player the player thats being checked
			 */
			std::vector<Point> get_obstacle_boundaries(AI::Nav::W::World world, AI::Nav::W::Player player, unsigned int added_flags);

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
			AI::Timestamp get_next_ts(AI::Timestamp now, Point &p1, Point &p2, Point target_velocity);

			/**
			 * handle the cases where ball is not moving or, moving towards target slowly (when robot push the ball away)
			 */
		
			bool intercept_flag_stationary_ball_handler(AI::Nav::W::World world, AI::Nav::W::Player player);

			/**
			 * Calculate the best position to intersect the ball
			 * 
			 * \param[in] world the world, passed into rrt_planner in the code
			 *
			 * \param[in] player the robot that performing the intersection
			 */
			bool intercept_flag_handler(AI::Nav::W::World world, AI::Nav::W::Player player, AI::Nav::RRT::PlayerData::Ptr player_data);

			
			/**
			 * assign the player to go to its current position, robot may still move depend on how the robot controller deal with it
			 *
			 * \param[in] player in interest
			 */
			//void make_stationary( AI::Nav::W::World world, AI::Nav::W::Player player );
		}
	}
}

#endif

