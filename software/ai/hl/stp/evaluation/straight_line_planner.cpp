#include "ai/hl/stp/evaluation/straight_line_planner.h"
#include "ai/hl/stp/evaluation/plan_util.h"
#include "geom/shapes.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;
namespace Plan = AI::HL::STP::Evaluation::Plan;
namespace SLP = AI::HL::STP::Evaluation::SLP;
using namespace Geom;

namespace {
	const constexpr double NEW_POINT_BUFFER = 0.01;
	const constexpr Point NULL_POINT = Point(-999.9, -999.9);
	const constexpr Circle NULL_CIRCLE = Circle(NULL_POINT, 0);
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

	std::vector<Point> path = straight_line_plan_helper(player.position(), target, obstacles, SLP::MODE_BOTH, 30);
	if(path.empty()) {
		LOG_INFO(u8"failed to find a path with SLP!!!!!");
	}
	return path;
}

std::vector<Point> Evaluation::SLP::straight_line_plan_helper(const Point &start, const Point &target, const std::vector<Geom::Circle> &obstacles, SLP::PlanMode mode, int maxDepth) {
	if(maxDepth < 0) {
		return std::vector<Point>();
	}

	Circle firstCollision = SLP::getFirstCollision(start, target, obstacles);
	if(firstCollision == NULL_CIRCLE) {
		return std::vector<Point> {target};
	}

	if(mode == SLP::MODE_LEFT) {
		std::vector<Circle> obstacleGroup = SLP::getGroupOfObstacles(firstCollision, obstacles);
		Point leftPerpPoint = NULL_POINT;
		double leftPerpPointDist = -1;

		// The perp() function returns points facing counterclockwise
		for(Circle ob : obstacleGroup) {
			Point perpPoint = ob.origin + (target - start).perp().norm(ob.radius + NEW_POINT_BUFFER);
			double dist = Geom::dist(Line(start, target), perpPoint);

			if(!point_is_to_right_of_line(Seg(start, target), perpPoint) &&
					(leftPerpPoint == NULL_POINT || dist > leftPerpPointDist) &&
					closest_lineseg_point(perpPoint, start, target) != start &&
					closest_lineseg_point(perpPoint, start, target) != target) {
				leftPerpPoint = perpPoint;
				leftPerpPointDist = dist;
			}
		}

		std::vector<Point> planFirstPart = SLP::straight_line_plan_helper(start, leftPerpPoint, obstacles, SLP::MODE_LEFT, maxDepth - 1);
		std::vector<Point> planSecondPart = SLP::straight_line_plan_helper(leftPerpPoint, target, obstacles, SLP::MODE_LEFT, maxDepth - 1);
		if(planFirstPart.empty() || planSecondPart.empty()) {
			return std::vector<Point>();
		}else {
			planFirstPart.insert(planFirstPart.end(), planSecondPart.begin(), planSecondPart.end());
			return planFirstPart;
		}
	}else if(mode == SLP::MODE_RIGHT) {
		std::vector<Circle> obstacleGroup = SLP::getGroupOfObstacles(firstCollision, obstacles);
		Point rightPerpPoint = NULL_POINT;
		double rightPerpPointDist = -1;

		// The perp() function returns points facing counterclockwise
		for(Circle ob : obstacleGroup) {
			Point perpPoint = ob.origin - (target - start).perp().norm(ob.radius + NEW_POINT_BUFFER);
			double dist = Geom::dist(Line(start, target), perpPoint);

			if(point_is_to_right_of_line(Seg(start, target), perpPoint) &&
					(rightPerpPoint == NULL_POINT || dist > rightPerpPointDist) &&
					closest_lineseg_point(perpPoint, start, target) != start &&
					closest_lineseg_point(perpPoint, start, target) != target) {
				rightPerpPoint = perpPoint;
				rightPerpPointDist = dist;
			}
		}

		std::vector<Point> planFirstPart = SLP::straight_line_plan_helper(start, rightPerpPoint, obstacles, SLP::MODE_RIGHT, maxDepth - 1);
		std::vector<Point> planSecondPart = SLP::straight_line_plan_helper(rightPerpPoint, target, obstacles, SLP::MODE_RIGHT, maxDepth - 1);
		if(planFirstPart.empty() || planSecondPart.empty()) {
			return std::vector<Point>();
		}else {
			planFirstPart.insert(planFirstPart.end(), planSecondPart.begin(), planSecondPart.end());
			return planFirstPart;
		}
	}else if(mode == SLP::MODE_BOTH) {
		std::vector<Circle> obstacleGroup = SLP::getGroupOfObstacles(firstCollision, obstacles);
		Point leftPointStart = SLP::getGroupTangentPoints(start, obstacleGroup, NEW_POINT_BUFFER).first;
		Point leftPointTarget = SLP::getGroupTangentPoints(target, obstacleGroup, NEW_POINT_BUFFER).second;
		Point rightPointStart = SLP::getGroupTangentPoints(start, obstacleGroup, NEW_POINT_BUFFER).second;
		Point rightPointTarget = SLP::getGroupTangentPoints(target, obstacleGroup, NEW_POINT_BUFFER).first;

		std::vector<Point> leftPath1 = SLP::straight_line_plan_helper(start, leftPointStart, obstacles, SLP::MODE_CLOSEST_SIDE, maxDepth - 1);
		std::vector<Point> leftPath2 = SLP::straight_line_plan_helper(leftPointStart, leftPointTarget, obstacles, SLP::MODE_LEFT, maxDepth - 1);
		std::vector<Point> leftPath3 = SLP::straight_line_plan_helper(leftPointTarget, target, obstacles, SLP::MODE_BOTH, maxDepth - 1);

		std::vector<Point> rightPath1 = SLP::straight_line_plan_helper(start, rightPointStart, obstacles, SLP::MODE_CLOSEST_SIDE, maxDepth - 1);
		std::vector<Point> rightPath2 = SLP::straight_line_plan_helper(rightPointStart, rightPointTarget, obstacles, SLP::MODE_RIGHT, maxDepth - 1);
		std::vector<Point> rightPath3 = SLP::straight_line_plan_helper(rightPointTarget, target, obstacles, SLP::MODE_BOTH, maxDepth - 1);

		if(leftPath1.empty() || leftPath2.empty() || leftPath3.empty()) {
			if(rightPath1.empty() || rightPath2.empty() || rightPath3.empty()) {
				// Neither direction is valid
				return std::vector<Point>();
			}else {
				// Only the right path is valid
				rightPath1.insert(rightPath1.end(), rightPath2.begin(), rightPath2.end());
				rightPath1.insert(rightPath1.end(), rightPath3.begin(), rightPath3.end());
				return rightPath1;
			}
		}else {
			if(rightPath1.empty() || rightPath2.empty() || rightPath3.empty()) {
				// Only the left path is valid
				leftPath1.insert(leftPath1.end(), leftPath2.begin(), leftPath2.end());
				leftPath1.insert(leftPath1.end(), leftPath3.begin(), leftPath3.end());
				return leftPath1;
			}else {
				// Both paths are valid so choose the best one
				leftPath1.insert(leftPath1.end(), leftPath2.begin(), leftPath2.end());
				leftPath1.insert(leftPath1.end(), leftPath3.begin(), leftPath3.end());

				rightPath1.insert(rightPath1.end(), rightPath2.begin(), rightPath2.end());
				rightPath1.insert(rightPath1.end(), rightPath3.begin(), rightPath3.end());

				if(SLP::getPathScore(start, leftPath1) > SLP::getPathScore(start, rightPath1)) {
					return leftPath1;
				}else {
					return rightPath1;
				}
			}
		}
	}else if(mode == SLP::MODE_CLOSEST_SIDE) {
		Point closestPoint = closest_lineseg_point(firstCollision.origin, start, target);
		closestPoint = firstCollision.origin + (closestPoint - firstCollision.origin).norm(firstCollision.radius + NEW_POINT_BUFFER);
		std::vector<Point> path1 = SLP::straight_line_plan_helper(start, closestPoint, obstacles, SLP::MODE_CLOSEST_SIDE, maxDepth - 1);
		std::vector<Point> path2 = SLP::straight_line_plan_helper(closestPoint, target, obstacles, SLP::MODE_CLOSEST_SIDE, maxDepth - 1);
		if(path1.empty() || path2.empty()) {
			return std::vector<Point>();
		}else {
			path1.insert(path1.end(), path2.begin(), path2.end());
			return path1;
		}
	}else {
		// The planner got into an invalid mode. This should never happen.
		LOGF_INFO(u8"ERROR: The straight line planner is in an invalid mode: %1", mode);
		return std::vector<Point>();
	}
}

Circle Evaluation::SLP::getFirstCollision(const Point &start, const Point &end, const std::vector<Circle> &obstacles) {
	// TODO: add check for empty obstacles
	Circle closestObstacle = NULL_CIRCLE;

	for(Circle c : obstacles) {
		if(intersects(c, Seg(start, end))) {
			if(closestObstacle == NULL_CIRCLE || (c.origin - start).len() < (closestObstacle.origin - start).len()) {
				closestObstacle = c;
			}
		}
	}

	return closestObstacle;
}

std::vector<Circle> Evaluation::SLP::getGroupOfObstacles(const Circle &obstacle, const std::vector<Circle> &obstacles) {
	std::vector<Circle> touchingObstacles = {obstacle};
	std::vector<Circle> obstaclesToAdd;
	# warning this can definitely be optimized. Check for empty list of obstacles, maybe use std::set and loop on obstaclesToAdd instead of whole list again
	while(true) {
		obstaclesToAdd.clear();

		for(Circle c : touchingObstacles) {
			for(Circle ob : obstacles) {
				# warning a wrapper function to check if an object is in a vector would be nice
				if(Geom::intersects(c, ob) && std::find(touchingObstacles.begin(), touchingObstacles.end(), ob) == touchingObstacles.end()) {
					obstaclesToAdd.push_back(ob);
				}
			}
		}

		if(obstaclesToAdd.empty()) {
			break;
		}

		touchingObstacles.insert(touchingObstacles.end(), obstaclesToAdd.begin(), obstaclesToAdd.end());
	}

	return touchingObstacles;
}

std::pair<Point, Point> Evaluation::SLP::getGroupTangentPoints(const Point &start, const std::vector<Circle> &obstacles, double buffer) {
	if(obstacles.empty()) {
		return std::make_pair(NULL_POINT, NULL_POINT);
	}

	Point tangent1 = NULL_POINT;
	Point tangent2 = NULL_POINT;
	Circle obstacle1 = NULL_CIRCLE;

	for(Circle c : obstacles) {
		std::vector<Point> tans = {get_circle_tangent_points(start, c, buffer).first, get_circle_tangent_points(start, c, buffer).second};
		for(Point t : tans) {
			Circle collision = Evaluation::SLP::getFirstCollision(start, start + (t - start).norm(1000), obstacles);
			if(tangent1 == NULL_POINT && collision == NULL_CIRCLE) {
				tangent1 = t;
				obstacle1 = c;
				continue;
			}

			if(tangent1 != NULL_POINT && collision == NULL_CIRCLE) {
				tangent2 = t;
				break;
			}
		}
	}

	if(is_clockwise(start - obstacle1.origin, tangent1 - obstacle1.origin)) {
		return std::make_pair(tangent1, tangent2);
	}else {
		return std::make_pair(tangent2, tangent1);
	}
}

double Evaluation::SLP::getPathScore(const Point &start, const std::vector<Point> &path) {
	if(path.empty()) {
		return 0.0;
	}

	int numSegments = static_cast<int>(path.size());
	double pathLength = (path[0] - start).len();
	for(unsigned int i = 0; i < path.size() - 1; i++) {
		pathLength += (path[i] - path[i+1]).len();
	}

	return 100 / (pathLength + numSegments + 1);
}
