#ifndef AI_HL_STP_EVALUATION_STRAIGHT_LINE_PLANNER_H
#define AI_HL_STP_EVALUATION_STRAIGHT_LINE_PLANNER_H

#include "ai/hl/stp/world.h"
#include "geom/point.h"
#include "geom/shapes.h"
#include <vector>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
                namespace SLP{
                	/**
                	 * Returns an ordered list of points representing a path from the player's position to the target
                	 * Will call rrt planner if no path can be found
                	 */
                	std::vector<Point> straight_line_plan(World world, Player player, Point target, AI::Flags::MoveFlags added_flags = AI::Flags::MoveFlags::NONE);

                	/**
                	 * Returns an ordered list of points representing the path from start to target, while avoiding obstacles
                	 */
                	std::vector<Point> straight_line_plan_helper(const Point &start, const Point &target, const std::vector<Geom::Circle> &obstacles, int maxDepth);

                	/**
                	 * Given a point and a list of obstacles, return a vector of points representing the group of obstacles
                	 * around obstacle. Obstacles are considered to be a group if they are close enough together that a robot
                	 * cannot pass between them
                	 */
                	std::vector<Geom::Circle> getGroupOfObstacles(const Geom::Circle &obstacle, const std::vector<Geom::Circle> &all_obstacles);

                	/**
                	 * Returns true if the line between start and target goes between any 2 (or more) points in obstacleGroup
                	 */
                	bool lineSplitsGroup(const Point &start, const Point &target, const std::vector<Geom::Circle> &obstacleGroup);

                	/**
                	 * Returns a pair of points that represent the points at the edges of a group of points
                	 */
                	std::pair<Point, Point> getGroupCollisionEndpoints(const Point &start, const Point &target, const std::vector<Geom::Circle> &obstacleGroup);

                	/**
                	 * Returns the first point from obstacles that the line from start to end collides with
                	 */
                	Geom::Circle getFirstCollision(const Point &start, const Point &end, const std::vector<Geom::Circle> &obstacles);
                }
            }
        }
    }
}

#endif
