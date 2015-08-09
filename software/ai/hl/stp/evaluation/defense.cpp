#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/enemy.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/param.h"
#include "geom/angle.h"
#include "ai/hl/stp/predicates.h"

#include <vector>

using namespace AI::HL::W;
using namespace AI::HL::STP::Evaluation;
using namespace AI::HL::STP;
using namespace Geom;

namespace {
	BoolParam defense_follow_enemy_baller(u8"defense protect against baller", u8"AI/HL/STP/defense", true);

	BoolParam goalie_hug_switch(u8"goalie hug switch", u8"AI/HL/STP/defense", true);

	DoubleParam max_goalie_dist(u8"max goalie dist from goal (robot radius)", u8"AI/HL/STP/defense", 3.0, 0.0, 10.0);

	DoubleParam robot_shrink(u8"shrink robot radius", u8"AI/HL/STP/defense", 1.1, 0.1, 2.0);
	DoubleParam ball2side_ratio(u8"ball2side ratio", u8"AI/HL/STP/defense", 0.7, 0, 10);

	BoolParam open_net_dangerous(u8"open net enemy is dangerous", u8"AI/HL/STP/defense", true);

	DoubleParam tdefend_dist(u8"Distance between the tdefenders", u8"AI/HL/STP/tdefend", 2.25, 1.0, 3.0);

	// The closest distance players allowed to the ball
	// DO NOT make this EXACT, instead, add a little tolerance!
	const double AVOIDANCE_DIST = AI::Util::BALL_STOP_DIST + Robot::MAX_RADIUS + Ball::RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const Angle AVOIDANCE_ANGLE = 2.0 * Angle::of_radians(std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST));
	
	/**
	 * ssshh... global state
	 * DO NOT TOUCH THIS unless you know what you are doing.
	 */
	bool goalie_top = true;

	/*
	   template<int N>
	   class EvaluateDefense final : public Cacheable<Point, CacheableNonKeyArgs<AI::HL::W::World>> {
	   protected:
	   std::array<Point, N> compute(AI::HL::W::World world) override;
	   bool goalie_top;
	   };
	 */

	std::array<Point, MAX_DEFENDERS + 1> waypoints;

	std::array<Point, MAX_DEFENDERS + 1> compute(World world) {
		const Field &field = world.field();

		if (world.ball().position().y > field.goal_width() / 2) {
			goalie_top = !goalie_hug_switch;
		} else if (world.ball().position().y < -field.goal_width() / 2) {
			goalie_top = goalie_hug_switch;
		}

		// list of points to defend, by order of importance
		std::vector<Point> waypoint_defenders;

		//list of points to intercept enemy passing lanes
		std::vector<Point> waypoint_passing_lanes;

		// there is cone ball to goal sides, bounded by 1 rays.
		// the side that goalie is going to guard is goal_side
		// the opposite side is goal_opp
		// a defender will guard goal_opp

		Point ball_pos = world.ball().position();
		if (defense_follow_enemy_baller) {
			Robot robot = calc_enemy_baller(world);
			if (robot) {
				ball_pos = robot.position();
			}
		}

		const Point goal_side = goalie_top ? Point(-field.length() / 2, field.goal_width() / 2) : Point(-field.length() / 2, -field.goal_width() / 2);
		const Point goal_opp = goalie_top ? Point(-field.length() / 2, -field.goal_width() / 2) : Point(-field.length() / 2, field.goal_width() / 2);

		// now calculate where you want the goalie to be
		Point waypoint_goalie;

		const double radius = Robot::MAX_RADIUS * robot_shrink;
		bool second_needed = true;
		{
			// distance on the goalside - ball line that the robot touches
			const Point ball2side = goal_side - ball_pos;
			const Point touch_vec = -ball2side.norm(); // side to ball
			const double touch_dist = std::min(-ball2side.x * ball2side_ratio, max_goalie_dist * Robot::MAX_RADIUS);
			const Point perp = (goalie_top) ? touch_vec.rotate(-Angle::quarter()) : touch_vec.rotate(Angle::quarter());
			waypoint_goalie = goal_side + touch_vec * touch_dist + perp * radius;

			// prevent the goalie from entering the goal area
			waypoint_goalie.x = std::max(waypoint_goalie.x, -field.length() / 2 + radius);

			second_needed = dist(waypoint_goalie, Seg(ball_pos, goal_opp)) > radius;
		}

		// first defender will block the remaining cone from the ball
		if (second_needed) {
			Point D1 = calc_block_cone_defender(goal_side, goal_opp, ball_pos, waypoint_goalie, radius);

			if (ball_on_net(world)) {
				D1 = calc_block_cone(goal_side, goal_opp, ball_pos, radius);
			}

			bool blowup = false;
			if (D1.x < Robot::MAX_RADIUS - field.length() / 2 + field.defense_area_stretch()) {
				blowup = true;
			}
			if (std::fabs(D1.y) > field.width() / 4) {
				blowup = true;
			}
			if (blowup) {
				D1 = (field.friendly_goal() + ball_pos) / 2;
			}
			waypoint_defenders.push_back(D1);
		}

		std::vector<Robot> enemies = enemies_by_grab_ball_dist();

		// sort enemies by distance to own goal
		// std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot>(field.friendly_goal()));

		std::vector<Robot> threat;

		if (open_net_dangerous && second_needed) {
			std::vector<Point> obstacles;
			obstacles.push_back(waypoint_goalie);
			obstacles.push_back(waypoint_defenders[0]);

			for (size_t i = 0; i < enemies.size(); ++i) {
				if (calc_enemy_best_shot_goal(world.field(), obstacles, enemies[i].position()).second > enemy_shoot_accuracy) {
					threat.push_back(enemies[i]);
				}
			}
			for (size_t i = 0; i < enemies.size(); ++i) {
				if (!(calc_enemy_best_shot_goal(world.field(), obstacles, enemies[i].position()).second > enemy_shoot_accuracy)) {
					threat.push_back(enemies[i]);
				}
			}
		} else {
			threat = enemies;
		}

		//figure out positions to stop the passing lanes

		for (size_t i = 0; i < threat.size(); i++) {
			waypoint_passing_lanes.push_back(threat[i].position());
		}


		// next two defenders block nearest enemy sights to goal if needed
		// enemies with ball possession are ignored (they should be handled above)
		for (size_t i = 0; i < threat.size() && waypoint_defenders.size() < MAX_DEFENDERS; ++i) {
// HACK
			if (defense_follow_enemy_baller) {
				Robot robot = calc_enemy_baller(world);
				if (robot && threat[i] == robot) {
					continue;
				}
			}

#warning A HACK FOR NOW, may intefere with baller above
			double ball_diff = (ball_pos - threat[i].position()).len();
			if (ball_diff < Robot::MAX_RADIUS + Ball::RADIUS) {
				continue;
			}

			// TODO: check if enemy can shoot the ball from here
			// if so, block it

			/**
			 * block the passing lanes with the next few robots
			 */
			Point D(0,0);
			if (world.ball().position().x < 0)
			{
				bool blowup = false;
				D = closest_lineseg_point(world.field().friendly_goal(), world.ball().position(), threat[i].position());
				if (D.x < Robot::MAX_RADIUS - field.length() / 2 + field.defense_area_stretch()) {
					blowup = true;
					}
				if (std::fabs(D.y) > field.width() / 4) {
					blowup = true;
				}
				if (blowup) {
					D = (field.friendly_goal() + threat[i].position()) / 2;
				}
			}
			else {
				/*
				 * The following block of code calculates the the best place to put extra defenders to block the robots
				 * from shooting the ball from one touch passes. instead, we try to block passing lanes
				 */
				bool blowup = false;
				D = calc_block_cone(world.ball().position(), world.ball().position(), threat[i].position(), radius);
				if (D.x < Robot::MAX_RADIUS - field.length() / 2 + field.defense_area_stretch()) {
					blowup = true;
				}
				if (std::fabs(D.y) > field.width() / 4) {
					blowup = true;
				}
				if (blowup) {
					D = (field.friendly_goal() + threat[i].position()) / 2;
				}
			}
			waypoint_defenders.push_back(D);
		}

		// there are too few threat, this is strange
		while (waypoint_defenders.size() < MAX_DEFENDERS) {
			waypoint_defenders.push_back((field.friendly_goal() + ball_pos) / 2);
		}

		std::array<Point, MAX_DEFENDERS + 1> waypoints;
		waypoints[0] = waypoint_goalie;
		for (std::size_t i = 0; i < MAX_DEFENDERS; ++i) {
			waypoints[i + 1] = waypoint_defenders[i];
		}
		return waypoints;
	}

	Point tdefender_block_ball(World world, const unsigned index) {
		Point dirToGoal, target;
		dirToGoal = (world.field().friendly_goal() - world.ball().position()).norm();
		target = world.field().friendly_goal() - (6 * (index + 1) * Robot::MAX_RADIUS * dirToGoal);
		if (index == 3) {
			// same defense layer as tdefender1
			target = world.field().friendly_goal() - (6 * (1.6) * Robot::MAX_RADIUS * dirToGoal);
		}
		Point t = target;
		if (world.ball().position().y < 0.0) {
			if (index < 2) {
				target = Point(t.x, t.y + tdefend_dist * Robot::MAX_RADIUS);
			} else {
				target = Point(t.x, t.y - tdefend_dist * Robot::MAX_RADIUS);
			}
		} else {
			if (index < 2) {
				target = Point(t.x, t.y - tdefend_dist * Robot::MAX_RADIUS);
			} else {
				target = Point(t.x, t.y + tdefend_dist * Robot::MAX_RADIUS);
			}
		}
		return target;
	}

	Point tdefender_block_enemy(World world, Point r, const unsigned index) {
		Point dirToGoal;
		dirToGoal = (world.field().friendly_goal() - r).norm();
		Point block = world.field().friendly_goal() - (6 * (index + 1) * Robot::MAX_RADIUS * dirToGoal);
		if (index == 3) {
			block = world.field().friendly_goal() - (6 * (1.6) * Robot::MAX_RADIUS * dirToGoal);
		}
		return block;
	}
}

void AI::HL::STP::Evaluation::tick_defense(World world) {
	waypoints = compute(world);
}

const std::array<Point, MAX_DEFENDERS + 1> AI::HL::STP::Evaluation::evaluate_defense() {
	return waypoints;
}

bool AI::HL::STP::Evaluation::enemy_break_defense_duo(World world, const Robot enemy) {
	std::vector<Point> obstacles;
	obstacles.push_back(waypoints[0]);
	obstacles.push_back(waypoints[1]);

	return calc_enemy_best_shot_goal(world.field(), obstacles, enemy.position()).second > enemy_shoot_accuracy;
}

Point AI::HL::STP::Evaluation::evaluate_tdefense(World world, const unsigned index) {
	Point target;
	if (world.enemy_team().size() > index - 1) {
		std::vector<Robot> enemies = Evaluation::enemies_by_grab_ball_dist();
		Point r = enemies[index - 1].position();
		if (r.x < world.field().centre_circle_radius()) {
			target = tdefender_block_enemy(world, r, index);
		} else {
			target = tdefender_block_ball(world, index);
		}
	} else {
		target = tdefender_block_ball(world, index);
	}

	if (target.x < world.field().friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
		target.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
	}
	return target;
}

Point AI::HL::STP::Evaluation::evaluate_tdefense_line(World world, const Point p1, const Point p2, const double dist_min, const double dist_max){

	Point ball = world.ball().position(), target = (p1+p2)/2;
	double diff = (ball - target).len();
	if (diff > dist_max || diff < dist_min) return ball;
	return target;
}

Point AI::HL::STP::Evaluation::evaluate_shadow_enemy_point(World world, const int closest_enemy_index){
	Point enemy = Enemy::closest_ball(world, closest_enemy_index)->evaluate().position();
	Point ball = world.ball().position();
	Point destination = ball - enemy;
	Point rectangle[4];
	Point line_segment[2];

	line_segment[0] = enemy;
	line_segment[1] = world.field().friendly_goal();

	rectangle[0] = Point(world.field().friendly_goal().x + world.field().goal_width() / 2, world.field().friendly_goal().y);
	rectangle[1] = Point(world.field().friendly_goal().x - world.field().goal_width() / 2, world.field().friendly_goal().y);
	rectangle[2] = enemy;
	rectangle[3] = enemy;

	destination = destination.norm() * (AI::Util::BALL_STOP_DIST + Robot::MAX_RADIUS + Ball::RADIUS);
	destination = ball - destination;
	return destination;
}

std::vector<Point> AI::HL::STP::Evaluation::evaluate_enemy_passing_lanes(World world, const int highest_threat_index) {

	std::vector<Point> returnVector;
	return returnVector;
}
//
//	std::vector<Point> intercept_points;
//	std::vector<Robot> enemies = enemies_by_grab_ball_dist();
//
//	// sort enemies by distance to own goal
//	// std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot>(field.friendly_goal()));
//
//	std::vector<Robot> threat;
//
//	if (open_net_dangerous && second_needed) {
//		std::vector<Point> obstacles;
//		obstacles.push_back(waypoint_goalie);
//		obstacles.push_back(waypoint_defenders[0]);
//
//		for (size_t i = 0; i < enemies.size(); ++i) {
//			if (calc_enemy_best_shot_goal(world.field(), obstacles, enemies[i].position()).second > enemy_shoot_accuracy) {
//				threat.push_back(enemies[i]);
//			}
//		}
//		for (size_t i = 0; i < enemies.size(); ++i) {
//			if (!(calc_enemy_best_shot_goal(world.field(), obstacles, enemies[i].position()).second > enemy_shoot_accuracy)) {
//				threat.push_back(enemies[i]);
//			}
//		}
//	} else {
//		threat = enemies;
//	}
//
//	return intercept_points;



