#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/param.h"
#include "geom/angle.h"

#include <vector>

using namespace AI::HL::W;
using namespace AI::HL::STP::Evaluation;

namespace {
	BoolParam goalie_hug_switch("goalie hug switch", "STP/defense", true);

	DoubleParam max_goalie_dist("max goalie dist from goal (robot radius)", "STP/defense", 3.0, 0.0, 10.0);

	DoubleParam robot_shrink("shrink robot radius", "STP/defense", 1.1, 0.1, 2.0);
	DoubleParam ball2side_ratio("ball2side ratio", "STP/defense", 0.7, 0, 10);

	BoolParam open_net_dangerous("open net enemy is dangerous", "STP/defense", true);


	/**
	 * ssshh... global state
	 * DO NOT TOUCH THIS unless you know what you are doing.
	 */
	bool goalie_top = true;

	/*
	   template<int N>
	   class EvaluateDefense : public Cacheable<Point, CacheableNonKeyArgs<AI::HL::W::World &>> {
	   protected:
	   std::array<Point, N> compute(AI::HL::W::World &world);
	   bool goalie_top;
	   };
	 */

	std::array<Point, MAX_DEFENDERS + 1> compute(const World &world) {
		const Field &field = world.field();

		if (world.ball().position().y > field.goal_width() / 2) {
			goalie_top = !goalie_hug_switch;
		} else if (world.ball().position().y < -field.goal_width() / 2) {
			goalie_top = goalie_hug_switch;
		}

		// list of points to defend, by order of importance
		std::vector<Point> waypoint_defenders;

		// there is cone ball to goal sides, bounded by 1 rays.
		// the side that goalie is going to guard is goal_side
		// the opposite side is goal_opp
		// a defender will guard goal_opp
		const Point ball_pos = world.ball().position();
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
			const Point perp = (goalie_top) ? touch_vec.rotate(-M_PI / 2) : touch_vec.rotate(M_PI / 2);
			waypoint_goalie = goal_side + touch_vec * touch_dist + perp * radius;

			// prevent the goalie from entering the goal area
			waypoint_goalie.x = std::max(waypoint_goalie.x, -field.length() / 2 + radius);

			second_needed = lineseg_point_dist(waypoint_goalie, ball_pos, goal_opp) > radius;
		}

		// first defender will block the remaining cone from the ball
		if (second_needed) {
			Point D1 = calc_block_cone_defender(goal_side, goal_opp, ball_pos, waypoint_goalie, radius);
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

		std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

		// sort enemies by distance to own goal
		std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(field.friendly_goal()));

		std::vector<Robot::Ptr> threat;

		if (open_net_dangerous && second_needed) {

			std::vector<Point> obstacles;
			obstacles.push_back(waypoint_goalie);
			obstacles.push_back(waypoint_defenders[0]);

			for (size_t i = 0; i < enemies.size(); ++i) {
				if (calc_enemy_best_shot_goal(world.field(), obstacles, enemies[i]->position()).second > degrees2radians(enemy_shoot_accuracy)) {
					threat.push_back(enemies[i]);
				}
			}
			for (size_t i = 0; i < enemies.size(); ++i) {
				if (!calc_enemy_best_shot_goal(world.field(), obstacles, enemies[i]->position()).second > degrees2radians(enemy_shoot_accuracy)) {
					threat.push_back(enemies[i]);
				}
			}

		} else {
			threat = enemies;
		}

		// next two defenders block nearest enemy sights to goal if needed
		// enemies with ball possession are ignored (they should be handled above)
		for (size_t i = 0; i < threat.size() && waypoint_defenders.size() < MAX_DEFENDERS; ++i) {
			//if (AI::HL::Util::ball_close(world, threat[i])) {
			//continue;
			//}

			// TODO: check if enemy can shoot the ball from here
			// if so, block it

			bool blowup = false;
			Point D = calc_block_cone(goal_side, goal_opp, threat[i]->position(), radius);
			if (D.x < Robot::MAX_RADIUS - field.length() / 2 + field.defense_area_stretch()) {
				blowup = true;
			}
			if (std::fabs(D.y) > field.width() / 4) {
				blowup = true;
			}
			if (blowup) {
				D = (field.friendly_goal() + threat[i]->position()) / 2;
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
}

const std::array<Point, MAX_DEFENDERS + 1> AI::HL::STP::Evaluation::evaluate_defense(const World &world) {
	return compute(world);
}

bool AI::HL::STP::Evaluation::enemy_break_defense_duo(const World& world, const Robot::Ptr enemy) {
	auto waypoints = evaluate_defense(world);

	std::vector<Point> obstacles;
	obstacles.push_back(waypoints[0]);
	obstacles.push_back(waypoints[1]);

	return calc_enemy_best_shot_goal(world.field(), obstacles, enemy->position()).second > degrees2radians(enemy_shoot_accuracy);
}

