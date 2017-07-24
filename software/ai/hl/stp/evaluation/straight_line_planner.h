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
					enum PlanMode{
						MODE_BOTH, MODE_LEFT, MODE_RIGHT, MODE_CLOSEST_SIDE
					};

                	/**
                	 * Returns an ordered list of points representing a path from the player's position to the target
                	 * Will call rrt planner if no path can be found
                	 */
                	std::vector<Point> straight_line_plan(World world, Player player, Point target, AI::Flags::MoveFlags added_flags = AI::Flags::MoveFlags::NONE);

					# warning might not want to expose helpers later, or move them to eval

                	/**
                	 * Returns an ordered list of points representing the path from start to target, while avoiding obstacles
                	 */
                	std::vector<Point> straight_line_plan_helper(World world, Player player, const Point &start, const Point &target, const std::vector<Geom::Circle> &obstacles, PlanMode mode, int maxDepth);

                	/**
                	 * Returns the first point from obstacles that the line from start to end collides with
                	 */
                	Geom::Circle getFirstCollision(const Point &start, const Point &end, const std::vector<Geom::Circle> &obstacles, double buffer = 0.0);

                	Geom::Circle getCollision(const Point &point, const std::vector<Geom::Circle> &obstacles, double buffer = 0.0);

                	/**
                	 * Given a point and a list of obstacles, return a vector of points representing the group of obstacles
                	 * around obstacle. Obstacles are considered to be a group if they are close enough together that a robot
                	 * cannot pass between them
                	 */
                	std::pair<std::vector<Geom::Circle>, double> getGroupOfObstacles(const Geom::Circle &originObstacle, const std::vector<Geom::Circle> &obstacles, double buffer = 0.0);

                	/**
                	 * Returns the Points in obstacles that form tangent lines with the start points.
                	 */
                	std::pair<Point, Point> getGroupTangentPoints(const Point &start, const std::vector<Geom::Circle> &obstacles, double groupBuffer, double buffer = 0.0);

                	double getPathScore(const Point &start, const std::vector<Point> &path);
                }
            }
        }
    }
}

#endif
