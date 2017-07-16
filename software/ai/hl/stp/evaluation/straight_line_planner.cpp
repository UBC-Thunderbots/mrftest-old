#include "ai/hl/stp/evaluation/straight_line_planner.h"
#include "ai/hl/stp/evaluation/plan_util.h"
#include "geom/shapes.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;
namespace Plan = AI::HL::STP::Evaluation::Plan;
using namespace Geom;

namespace {
	const constexpr double NEW_POINT_BUFFER = 0.01;
	const constexpr Point DUMMY_VAL = Point(-99, -99);
}

std::vector<Point> Evaluation::SLP::straight_line_plan(World world, Player player, Point target, AI::Flags::MoveFlags added_flags) {
	std::vector<Circle> obstacles;

	/* All obstacles include an additional robot max radius to account for the planning robot as well
	 * This means that the CENTER POINT of the planning robot cannot go within the obstacles radius
	 */
	for(auto i : world.enemy_team()) {
		obstacles.push_back(Circle(i.position(), Plan::enemy(world, player) + Robot::MAX_RADIUS + NEW_POINT_BUFFER));
	}

	for(auto i : world.friendly_team()) {
		if(i.position() != player.position()) {
			obstacles.push_back(Circle(i.position(), Plan::friendly(player) + Robot::MAX_RADIUS + NEW_POINT_BUFFER));
		}
	}

	std::vector<Point> path = straight_line_plan_helper(player.position(), target, obstacles, 75);
	if(path.empty()) {
		LOG_INFO(u8"failed to find a path with SLP!!!!!");
	}
	return path;
}

std::vector<Point> Evaluation::SLP::straight_line_plan_helper(const Point &start, const Point &target, const std::vector<Circle> &obstacles, int maxDepth) {
	// Return an empty vector if we have gone too deep in the recursion. This likely means we encountered an edge case
	// and want to stop before we overflow
	LOGF_INFO(u8"max depth: %1", maxDepth);
	if(maxDepth < 0) {
		return std::vector<Point>();
	}

	Circle collision = getFirstCollision(start, target, obstacles);

	#warning Circle and other geom types should overrige == operator
	if(collision.origin == DUMMY_VAL) {
		// There is nothing in the way so we can go straight to the target
		return std::vector<Point>{target};
	}else {
		// There is something in the way. Figure out how to go around it
		Point newPoint = DUMMY_VAL;
		std::vector<Circle> group = getGroupOfObstacles(collision, obstacles);

		if(group.size() > 1 && lineSplitsGroup(start, target, group)) {
			// This is a group of obstacles, and the line is splitting them so we need to
			// find the endpoints we can use to get around
			std::pair<Point, Point> groupEndpoints = getGroupCollisionEndpoints(start, target, group);

			if((groupEndpoints.first - start).orientation().angle_diff((target - start).orientation()) <
					(groupEndpoints.second - start).orientation().angle_diff((target - start).orientation())) {
				newPoint = groupEndpoints.first;
			}else {
				newPoint = groupEndpoints.second;
			}
		}else {
			// This is not a group of obstacles, or we are already close enough to the edge of a group
			// that we can just stay to the same side to go around
			Point closestPoint = closest_lineseg_point(collision.origin, start, target);
			newPoint = collision.origin + (closestPoint - collision.origin).norm(collision.radius + NEW_POINT_BUFFER);

			// If we perfectly intersect a point, arbitrarily choose a side
			if((closestPoint - collision.origin).len() < 1e-6) {
				newPoint = collision.origin + (target - start).perp().norm(collision.radius + NEW_POINT_BUFFER);
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

std::vector<Circle> Evaluation::SLP::getGroupOfObstacles(const Circle &obstacle, const std::vector<Circle> &all_obstacles) {
	std::vector<Circle> touchingObstacles = {obstacle};

	for(Circle c : all_obstacles) {
		if(c.origin != obstacle.origin && intersects(obstacle, c)) {
			touchingObstacles.push_back(c);
		}
	}

	return touchingObstacles;
}

bool Evaluation::SLP::lineSplitsGroup(const Point &start, const Point &target, const std::vector<Circle> &obstacleGroup) {
	bool cw = false;
	bool ccw = false;

	for(Circle c : obstacleGroup) {
		if(is_clockwise(target - start, c.origin - start)) {
			cw = true;
		}
		if(!is_clockwise(target - start, c.origin - start)) {
			ccw = true;
		}
	}

	return cw && ccw;
}

std::pair<Point, Point> Evaluation::SLP::getGroupCollisionEndpoints(const Point &start, const Point &target, const std::vector<Circle> &obstacleGroup) {
	Circle cwObstacle = obstacleGroup[0];
	Circle ccwObstacle = obstacleGroup[0];

	for(Circle c : obstacleGroup) {
		if(is_clockwise(cwObstacle.origin - start, c.origin - start)) {
			cwObstacle = c;
		}
		if(!is_clockwise(ccwObstacle.origin - start, c.origin - start)) {
			ccwObstacle = c;
		}
	}

	// THE BUFFER MIGHT NEED TO BE INCLUDED IN THE RADIUS ALREADY WHEN CHECKING IF THINGS ARE VALID
	Point cwEndpoint = cwObstacle.origin - (cwObstacle.origin - start).perp().norm(cwObstacle.radius + NEW_POINT_BUFFER);
	Point ccwEndpoint = ccwObstacle.origin + (ccwObstacle.origin - start).perp().norm(ccwObstacle.radius + NEW_POINT_BUFFER);

	return std::pair<Point, Point>(cwEndpoint, ccwEndpoint);
}

Circle Evaluation::SLP::getFirstCollision(const Point &start, const Point &end, const std::vector<Circle> &obstacles) {
	Circle closestObstacle = Circle(DUMMY_VAL, 0);

	for(Circle c : obstacles) {
		if(intersects(c, Seg(start, end))) {
			if(closestObstacle.origin == DUMMY_VAL || (c.origin - start).len() < (closestObstacle.origin - start).len()) {
				closestObstacle = c;
			}
		}
	}

	return closestObstacle;
}
