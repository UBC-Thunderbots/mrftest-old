#include "ai/hl/stp/evaluation/straight_line_planner.h"
#include "ai/hl/stp/evaluation/plan_util.h"
#include "geom/shapes.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"
#include <assert.h>
#include <chrono>

using namespace AI::HL::STP;
namespace Plan = AI::HL::STP::Evaluation::Plan;
namespace SLP = AI::HL::STP::Evaluation::SLP;
using namespace Geom;

namespace {
	const constexpr Point NULL_POINT = Point(-9, -9);
	const constexpr Circle NULL_CIRCLE = Circle(NULL_POINT, 0);
}

DoubleParam NEW_POINT_BUFFER(u8"How far from obstacles new points are generated", u8"AI/Navigator", 0.02, 0.0, 100.0);
DoubleParam ROBOT_PROJECTION_FACTOR(u8"The fraction of robot velocity to project in front as an obstacle", u8"AI/Navigator", 0.1, 0.0, 10.0);

// TODO: take waypoints as param
std::vector<Point> Evaluation::SLP::straight_line_plan(World world, Player player, Point target, AI::Flags::MoveFlags added_flags) {
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();


	Point start = player.position();
	std::vector<Circle> obstacles;
//	return std::vector<Point> {target};

	/* All obstacles include an additional robot max radius to account for the planning robot as well
	 * This means that the CENTER POINT of the planning robot cannot go within the obstacles radius
	 */

	// TODO: point collision function should return distance in a pair

	// Friendly robots
	for(auto i : world.enemy_team()) {
		// taking the velocity divided by AI ticks per second
		double scaledVel = i.velocity().len() * ROBOT_PROJECTION_FACTOR;
		for(double d = 0.0; d <= scaledVel; d+= 0.02) {
			Point enemy = i.position() + i.velocity().norm(d);
			obstacles.push_back(Circle(enemy, Plan::enemy(world, player) + Robot::MAX_RADIUS + NEW_POINT_BUFFER));
		}
	}

	// Enemy robots
	for(auto i : world.friendly_team()) {
		if(i.position() != start) {
			// taking the velocity divided by AI ticks per second
			double scaledVel = i.velocity().len() * ROBOT_PROJECTION_FACTOR;
			for(double d = 0.0; d <= scaledVel; d+= 0.02) {
				Point friendly = i.position() + i.velocity().norm(d);
				obstacles.push_back(Circle(friendly, Plan::friendly(player) + Robot::MAX_RADIUS + NEW_POINT_BUFFER));
			}
		}
	}

	// Goal posts
	double goalPostAvoidDist = Plan::goal_post(player);
	obstacles.push_back(Circle(world.field().enemy_goal_boundary().first, goalPostAvoidDist));
	obstacles.push_back(Circle(world.field().enemy_goal_boundary().second, goalPostAvoidDist));
	obstacles.push_back(Circle(world.field().friendly_goal_boundary().first, goalPostAvoidDist));
	obstacles.push_back(Circle(world.field().friendly_goal_boundary().first, goalPostAvoidDist));

	// TODO: project this forward in ball's direction???
	// The ball
	double ballAvoidDist = Plan::get_ball_avoid_dist(player);
	obstacles.push_back(Circle(world.ball().position(), ballAvoidDist));

	// friendly defense area
	double friendlyDefenseAvoidDist = Plan::friendly_defense(world, player);
	int numFriendlyDefenseObstacles = 5; // MUST BE GREATER THAN 1
	double friendlyDeltaDist = world.field().defense_area_stretch() / numFriendlyDefenseObstacles - 1;
	for(int i = 0; i < numFriendlyDefenseObstacles; i++) {
		Point p = Point(-world.field().length() / 2, -world.field().defense_area_stretch() / 2 + i * friendlyDeltaDist);
		obstacles.push_back(Circle(p, friendlyDefenseAvoidDist));
	}

	// enemy defense area
	double enemyDefenseAvoidDist = Plan::friendly_kick(world, player);
	int numEnemyDefenseObstacles = 5; // MUST BE GREATER THAN 1
	double enemyDeltaDist = world.field().defense_area_stretch() / numEnemyDefenseObstacles - 1;
	for(int i = 0; i < numEnemyDefenseObstacles; i++) {
		Point p = Point(world.field().length() / 2, -world.field().defense_area_stretch() / 2 + i * enemyDeltaDist);
		obstacles.push_back(Circle(p, enemyDefenseAvoidDist));
	}

	// goal trespass
	// need play area bounds
	// need total area bounds
	// need own half
	// need penaly friendy and enemy

	// This will adjust the path if the start or target point are in invalid locations (they violate the flags)
	// In thie case, the first priority is for the player to leave the violation zone (in the case of start being invalid)
	// and getting as close as possible without being invalid (if the case of the target being invalid)
	Point prepend = NULL_POINT;
//	Circle startCollision = SLP::getCollision(start, obstacles);
//	if(startCollision != NULL_CIRCLE) {
//		std::vector<Circle> startCollisionGroup = SLP::getGroupOfObstacles(startCollision, obstacles, NEW_POINT_BUFFER);
//		std::vector<Point> possiblePoints;
//		double closestDist = 99999;
//		int numChecks = 60;
//		for(int i = 0; i < numChecks; i++) {
//			for(Circle c : startCollisionGroup) {
//				std::vector<Point> intersectPoints = line_circle_intersect(c.origin, c.radius + NEW_POINT_BUFFER, start, start + Point(1, 0).rotate(Angle::full() / numChecks * i));
//				possiblePoints.insert(possiblePoints.begin(), intersectPoints.begin(), intersectPoints.end());
//			}
//		}
//
//		// find the closest point that's valid that we can move to
//		for(Point p : possiblePoints) {
//			bool valid = SLP::getCollision(p, startCollisionGroup) == NULL_CIRCLE;
//			if(valid && (prepend == NULL_POINT || (p - start).lensq() < (prepend - start).lensq())) {
//				prepend = p;
//			}
//		}
//
//		prepend = startCollision.origin + (start - startCollision.origin).norm(startCollision.radius + NEW_POINT_BUFFER);
//	}

	Point newTarget = NULL_POINT;
	Circle targetCollision = SLP::getCollision(target, obstacles);
	if(targetCollision != NULL_CIRCLE) {
		std::vector<Circle> targetCollisionGroup = SLP::getGroupOfObstacles(targetCollision, obstacles, NEW_POINT_BUFFER);
		std::vector<Point> possiblePoints;
		double closestDist = 99999;
		int numChecks = 60;
		for(int i = 0; i < numChecks; i++) {
			for(Circle c : targetCollisionGroup) {
				std::vector<Point> intersectPoints = line_circle_intersect(c.origin, c.radius + NEW_POINT_BUFFER, target, target + Point(1, 0).rotate(Angle::full() / numChecks * i));
				possiblePoints.insert(possiblePoints.begin(), intersectPoints.begin(), intersectPoints.end());
			}
		}

		// find the closest point that's valid that we can move to
		for(Point p : possiblePoints) {
			bool valid = SLP::getCollision(p, targetCollisionGroup) == NULL_CIRCLE;
			if(valid && (newTarget == NULL_POINT || (p - target).lensq() < (newTarget - target).lensq())) {
				newTarget = p;
			}
		}

		newTarget = targetCollision.origin + (start - targetCollision.origin).norm(targetCollision.radius + NEW_POINT_BUFFER);
		target = newTarget;
	}

	LOG_INFO(u8"STARTING THE RECUSIVE SLP");
	std::vector<Point> path;
	if(prepend == NULL_POINT) {
		 path = straight_line_plan_helper(start, target, obstacles, SLP::MODE_BOTH, 30);
	}else {
		path = straight_line_plan_helper(prepend, target, obstacles, SLP::MODE_BOTH, 30);
		path.insert(path.begin(), prepend);
	}
	LOG_INFO(u8"DONE RECURSIVE SLP");

	if(path.empty()) {
		LOG_INFO(u8"failed to find a path with SLP!!!!!");
	}

	std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
	auto slp_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();
	LOGF_INFO(u8"SLP TIME: %1", slp_time);

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
		std::vector<Circle> obstacleGroup = SLP::getGroupOfObstacles(firstCollision, obstacles, 1.5 * NEW_POINT_BUFFER);
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
		std::vector<Circle> obstacleGroup = SLP::getGroupOfObstacles(firstCollision, obstacles, 1.5 * NEW_POINT_BUFFER);
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
		std::vector<Circle> obstacleGroup = SLP::getGroupOfObstacles(firstCollision, obstacles, 1.5 * NEW_POINT_BUFFER);
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

Circle Evaluation::SLP::getCollision(const Point &point, const std::vector<Circle> &obstacles) {
	// TODO: add check for empty obstacles
	Circle closestObstacle = NULL_CIRCLE;

	for(Circle c : obstacles) {
		if(contains(c, point)) {
			if(closestObstacle == NULL_CIRCLE || (c.origin - point).len() < (closestObstacle.origin - point).len()) {
				closestObstacle = c;
			}
		}
	}

	return closestObstacle;
}

std::vector<Circle> Evaluation::SLP::getGroupOfObstacles(const Circle &obstacle, const std::vector<Circle> &obstacles, double buffer) {
	std::vector<Circle> touchingObstacles = {obstacle};
	std::vector<Circle> obstaclesToAdd;
	# warning this can definitely be optimized. Check for empty list of obstacles, maybe use std::set and loop on obstaclesToAdd instead of whole list again
	while(true) {
		obstaclesToAdd.clear();

		for(Circle c : touchingObstacles) {
			for(Circle ob : obstacles) {
				# warning a wrapper function to check if an object is in a vector would be nice
				if(Geom::intersects(Circle(c.origin, c.radius + buffer), ob) && std::find(touchingObstacles.begin(), touchingObstacles.end(), ob) == touchingObstacles.end()) {
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

//	assert(tangent1 != NULL_POINT);
//	assert(tangent2 != NULL_POINT);

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
