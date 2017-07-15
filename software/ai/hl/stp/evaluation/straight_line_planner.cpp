#include "ai/hl/stp/evaluation/straight_line_planner.h"
#include "ai/hl/stp/evaluation/plan_util.h"
#include "geom/shapes.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"
#include <memory>
#include <algorithm>

using namespace AI::HL::STP;
using namespace AI::HL::STP::Evaluation::Plan;
using namespace AI::Flags;

namespace {
	constexpr double AVOID_THRESHOLD = Robot::MAX_RADIUS / 2;
	constexpr double TOTAL_AVOID_DIST = 2 * Robot::MAX_RADIUS + AVOID_THRESHOLD;
	constexpr double NEW_POINT_BUFFER = 0.02;
	constexpr double TOTAL_AVOID_DIST_WITH_BUFFER = TOTAL_AVOID_DIST + NEW_POINT_BUFFER;
	constexpr Point DUMMY_VAL = Point(-99, -99);
}

namespace AI {
namespace HL {
namespace STP {
namespace Evaluation {
namespace SLP {
	std::vector<Point> straight_line_plan(World world, Player player, Point target, AI::Flags::MoveFlags added_flags) {
		std::vector<Point> obstacles;
		for(auto i : world.enemy_team()) {
			obstacles.push_back(i.position());
		}
		for(auto i : world.friendly_team()) {
			if(!(i.position() == player.position())) {
				obstacles.push_back(i.position());
			}
		}

		return straight_line_plan_helper(player.position(), target, obstacles);
	}

	std::vector<Point> straight_line_plan_helper(const Point &start, const Point &target, const std::vector<Point> &obstacles, int maxDepth) {
		if(maxDepth < 0) {
			return std::vector<Point>();
		}

		Point collision = getFirstCollision(start, target, obstacles);
		if(collision == DUMMY_VAL) {
			return std::vector<Point>{target};
		}else {
			Point newPoint = DUMMY_VAL;
			std::vector<Point> group = getGroupOfPoints(collision, obstacles);
			if(group.size() > 1 && lineSplitsGroup(start, target, group)) {
				// This is a group of obstacles
				std::pair<Point, Point> groupEndpoints = getGroupCollisionEndpoints(start, target, group);
				if((groupEndpoints.first - start).orientation().angle_diff((target - start).orientation()) <
						(groupEndpoints.second - start).orientation().angle_diff((target - start).orientation())) {
					newPoint = groupEndpoints.first;
				}else {
					newPoint = groupEndpoints.second;
				}
			}else {
				// This is not a group of obstacles
				Point closestPoint = closest_lineseg_point(collision, start, target);
				newPoint = collision + (closestPoint - collision).norm(TOTAL_AVOID_DIST_WITH_BUFFER);
				if((closestPoint - collision).len() < 1e-6) {
					newPoint = collision + (target - start).perp().norm(TOTAL_AVOID_DIST_WITH_BUFFER);
				}
			}
			std::vector<Point> v1 = straight_line_plan_helper(start, newPoint, obstacles, maxDepth - 1);
			std::vector<Point> v2 = straight_line_plan_helper(newPoint, target, obstacles, maxDepth - 1);
			v1.insert(v1.end(), v2.begin(), v2.end());
			if(v1.empty() || v2.empty()) {
				return std::vector<Point>();
			}else {
				return v1;
			}
		}
	}

	std::vector<Point> getGroupOfPoints(const Point &obstacle, const std::vector<Point> &all_obstacles) {
		std::vector<Point> touchingObstacles;
		touchingObstacles.push_back(obstacle);
		for(Point p : all_obstacles) {
			if(p != obstacle && (obstacle - p).len() < 2 * TOTAL_AVOID_DIST) {
				touchingObstacles.push_back(p);
			}
		}
		return touchingObstacles;
	}

	bool lineSplitsGroup(const Point &start, const Point &target, const std::vector<Point> &obstacleGroup) {
		bool cw = false;
		bool ccw = false;
		for(Point p : obstacleGroup) {
			if(is_clockwise(target - start, p - start)) {
				cw = true;
			}
			if(!is_clockwise(target - start, p - start)) {
				ccw = true;
			}
		}
		return cw && ccw;
	}

	std::pair<Point, Point> getGroupCollisionEndpoints(const Point &start, const Point &target, const std::vector<Point> &obstacleGroup) {
		Point cwPoint = obstacleGroup[0];
		Point ccwPoint = obstacleGroup[0];
		for(Point p : obstacleGroup) {
			if(is_clockwise(cwPoint - start, p - start)) {
				cwPoint = p;
			}
			if(!is_clockwise(ccwPoint - start, p - start)) {
				ccwPoint = p;
			}
		}

		Point cwEndpoint = cwPoint - (cwPoint - start).perp().norm(TOTAL_AVOID_DIST_WITH_BUFFER);
		Point ccwEndpoint = ccwPoint + (ccwPoint - start).perp().norm(TOTAL_AVOID_DIST_WITH_BUFFER);
		return std::pair<Point, Point>(cwEndpoint, ccwEndpoint);
	}

	Point getFirstCollision(const Point &start, const Point &end, const std::vector<Point> &obstacles) {
		Point closestObstacle = DUMMY_VAL;
		for(Point p : obstacles) {
			if(Geom::intersects(Geom::Circle(p, TOTAL_AVOID_DIST), Geom::Seg(start, end))) {
				if(closestObstacle == DUMMY_VAL || (p - start).len() < (closestObstacle - start).len()) {
					closestObstacle = p;
				}
			}
		}
		return closestObstacle;
	}
}
}
}
}
}

