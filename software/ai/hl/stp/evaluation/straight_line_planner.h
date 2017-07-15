#ifndef AI_HL_STP_EVALUATION_STRAIGHT_LINE_PLANNER_H
#define AI_HL_STP_EVALUATION_STRAIGHT_LINE_PLANNER_H

#include "ai/hl/stp/world.h"
#include "util/object_store.h"
#include "geom/param.h"
#include "geom/point.h"
#include <vector>

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
                namespace SLP{
                	std::vector<Point> straight_line_plan(World world, Player player, Point target, AI::Flags::MoveFlags added_flags = AI::Flags::MoveFlags::NONE);

                	std::vector<Point> straight_line_plan_helper(const Point &start, const Point &target, const std::vector<Point> &obstacles, int maxDepth = 200);

                	std::vector<Point> getGroupOfPoints(const Point &obstacle, const std::vector<Point> &all_obstacles);

                	bool lineSplitsGroup(const Point &start, const Point &target, const std::vector<Point> &obstacleGroup);

                	std::pair<Point, Point> getGroupCollisionEndpoints(const Point &start, const Point &target, const std::vector<Point> &obstacleGroup);

                	Point getFirstCollision(const Point &start, const Point &end, const std::vector<Point> &obstacles);

                }
            }
        }
    }
}


#endif
