#pragma once

#include "ai/hl/stp/world.h"
#include "geom/point.h"
#include "geom/util.h"
#include <utility>
#include <vector>
#include <cairomm/context.h>
#include <cairomm/refptr.h>

using namespace AI::Flags;

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
                namespace Plan{
					double play_area();

					double total_bounds_area();

					double enemy(World world, Robot player);

					double goal_post(Player player);

					double friendly(Player player, MovePrio obs_prio = MovePrio::MEDIUM);

					double ball_stop(Player player);

					double ball_tiny(Player player);

					double ball_regular(Player player);

					double friendly_defense(World world, Player player);

					double friendly_kick(World world, Player player);

					double own_half(Player player);

					double penalty_kick_friendly(Player player);

					double penalty_kick_enemy(Player player);

					double get_net_trespass(Point cur, Point dst, World world);

					double get_goal_post_trespass(Point cur, Point dst, World world, Player player);

					double get_enemy_trespass(Point cur, Point dst, World world);

					double get_play_area_boundary_trespass(Point cur, Point dst, World world);

					double get_total_bounds_trespass(Point cur, Point dst, World world);

					double get_friendly_trespass(Point cur, Point dst, World world, Player player);

					double get_ball_stop_trespass(Point cur, Point dst, World world, Player player);

					double get_defense_area_trespass(Point cur, Point dst, World world, Player player);

					double get_own_half_trespass(Point, Point dst, World world, Player player);

					double get_offense_area_trespass(Point cur, Point dst, World world, Player player);

					double get_ball_tiny_trespass(Point cur, Point dst, World world, Player player);

					double get_ball_regular_trespass(Point cur, Point dst, World world, Player player);

					double get_penalty_friendly_trespass(Point cur, Point dst, World world, Player player);

					double get_penalty_enemy_trespass(Point cur, Point dst, World world, Player player);

                    /**
                     * The absolute maximum value that a primitive parameter can take.
                     */
                    const double MAX_PRIM_PARAM = 10;

                    /**
                     * Estimates the time taken by a robot moving through each point in sequence.
                     */
                    double estimate_action_duration(std::vector<std::pair<Point, Angle>> path_points);

                    /**
                     * Finds where to go and when to get there in order to intercept the moving ball along the route to dst
                     */
                    std::pair<Point, AI::Timestamp> get_ramball_location(Point dst, World world, Player player);

                    // bool has_ramball_location(Point dst, World world, Player player);
                    
                    /**
                     * returns true if the destination is valid
                     */
                    bool valid_dst(Point dst, World world, Player player);

                    /**
                     * Returns true if the straight line path between cur & dst has a maximum level of rules violation
                     * exactly equal to the violation level of cur
                     */
                    bool valid_path(Point cur, Point dst, World world, Player player);

                    /**
                     * Returns true if the straight line path between cur & dst has a maximum level of rules violation
                     * exactly equal to the violation level of cur. This method allows for extra rules to be imposed via the "extra_flags"
                     * parameter
                     */
                    bool valid_path(Point cur, Point dst, World world, Player player, AI::Flags::MoveFlags extra_flags);

                    /**
                     * Returns true if the new path is significantly better than the old path
                     * If one path has more points than the other, they are only evaluated up
                     * to the length of the shorter path.
                     *
                     * Paths are considered better if they are shorter and their endpoint is closer
                     * to the given destination
                     */
                    bool isNewPathBetter(const std::vector<Point> &oldPath, const std::vector<Point> &newPath, const Point &target);

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
                    std::vector<Point> get_destination_alternatives(Point dst, World world, Player player);


                    /**
                     * returns a list of legal destinations that are on the boundaries of obstacles such as the ball
                     * or enemy robots. These set of points may be valuable as a search space for a navigator
                     *
                     * \param[in] world the world for field information
                     *
                     * \param[in] player the player thats being checked
                     */
                    std::vector<Point> get_obstacle_boundaries(World world, Player player);

                    /**
                     * returns a list of legal destinations that are on the boundaries of obstacles such as the ball
                     * or enemy robots. These set of points may be valuable as a search space for a navigator
                     *
                     * \param[in] world the world for field information
                     *
                     * \param[in] player the player thats being checked
                     */
                    std::vector<Point> get_obstacle_boundaries(World world, Player player, AI::Flags::MoveFlags added_flags);

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
                    bool intercept_flag_stationary_ball_handler(World world, Player player);
                }
            }
		}
	}
}
