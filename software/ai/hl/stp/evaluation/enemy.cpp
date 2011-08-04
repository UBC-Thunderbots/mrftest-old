#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "geom/util.h"
#include "ai/hl/util.h"
#include "geom/angle.h"

#include <algorithm>

using namespace AI::HL::STP;
namespace Util = AI::HL::Util;

DegreeParam AI::HL::STP::Evaluation::enemy_shoot_accuracy("Enemy shoot accuracy (degrees)", "STP/enemy", 1.0, 0.0, 90.0);

bool AI::HL::STP::Evaluation::enemy_can_shoot_goal(const World &world, Robot::Ptr enemy) {
	return calc_enemy_best_shot_goal(world, enemy).second > enemy_shoot_accuracy;
}

/*
   std::pair<Point, double> AI::HL::STP::Evaluation::calc_enemy_best_shot_target(const World &world, const Point &target_pos, const Robot::Ptr enemy, const double radius) {
    std::vector<Point> obstacles;
    const FriendlyTeam &friendly = world.friendly_team();
    for (std::size_t i = 0; i < friendly.size(); ++i) {
        obstacles.push_back(friendly.get(i)->position());
    }
    const EnemyTeam &enemies = world.enemy_team();
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        const Robot::Ptr erob = enemies.get(i);
        if (erob == enemy) {
            continue;
        }
        obstacles.push_back(erob->position());
    }

    return calc_enemy_best_shot_target(target_pos, obstacles, enemy->position(), radius);
   }
 */

/*
   std::pair<Point, double> AI::HL::STP::Evaluation::calc_enemy_best_shot_target(const Point &target_pos, const std::vector<Point> &obstacles, const Point &p, const double radius) {

   #warning due to an old HACK, angle_sweep_circle only works on positive side, so this is a hack because of a hack

    std::vector<Point> obs_rev = obstacles;
    for (std::size_t i = 0; i < obs_rev.size(); ++i) {
        obs_rev[i].x *= -1;
    }

    Point target_pos_rev = target_pos;
    target_pos_rev.x *= -1;

    Point p_rev = p;
    p_rev.x *= -1;

    auto ret = AI::HL::Util::calc_best_shot_target(target_pos_rev, obs_rev, p_rev, radius);
    ret.first.x *= -1;

    return ret;
   }
 */

std::pair<Point, Angle> Evaluation::calc_enemy_best_shot_goal(const Field &f, const std::vector<Point> &obstacles, const Point &p, const double radius) {
#warning due to an old HACK, angle_sweep_circle only works on positive side, so this is a hack because of a hack

	std::vector<Point> obs_rev = obstacles;
	for (std::size_t i = 0; i < obs_rev.size(); ++i) {
		obs_rev[i].x *= -1;
	}

	const Point p1 = Point(f.length() / 2.0, -f.goal_width() / 2.0);
	const Point p2 = Point(f.length() / 2.0, f.goal_width() / 2.0);

	auto ret = angle_sweep_circles(Point(-p.x, p.y), p1, p2, obs_rev, radius * Robot::MAX_RADIUS);
	ret.first.x *= -1;
	return ret;

	/*
	   const Point p1 = Point(-f.length() / 2.0, f.goal_width() / 2.0);
	   const Point p2 = Point(-f.length() / 2.0, -f.goal_width() / 2.0);
	   return angle_sweep_circles(p, p1, p2, obstacles, radius * Robot::MAX_RADIUS);
	 */
}

std::pair<Point, Angle> Evaluation::calc_enemy_best_shot_goal(const World &world, const Robot::Ptr enemy, const double radius) {
	std::vector<Point> obstacles;
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		obstacles.push_back(friendly.get(i)->position());
	}
	const EnemyTeam &enemies = world.enemy_team();
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		const Robot::Ptr erob = enemies.get(i);
		if (erob == enemy) {
			continue;
		}
		obstacles.push_back(erob->position());
	}
	return calc_enemy_best_shot_goal(world.field(), obstacles, enemy->position(), radius);
}

std::vector<Robot::Ptr> AI::HL::STP::Evaluation::get_passees(const World &world, Robot::Ptr robot) {
	const EnemyTeam &enemy = world.enemy_team();

	// don't count this robot
	std::vector<Robot::Ptr> passees;
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (robot == enemy.get(i)) {
			continue;
		}
		if (enemy_can_pass(world, robot, enemy.get(i))) {
			passees.push_back(enemy.get(i));
		}
	}
	return passees;
}

int AI::HL::STP::Evaluation::calc_enemy_pass(const World &world, const Robot::Ptr robot) {
	const EnemyTeam &enemy = world.enemy_team();

	auto threats = calc_enemy_threat(world);

	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (enemy.get(i) != robot) {
			continue;
		}

		return threats[i].passes_goal;
	}

	return 5;

	/*
	   const EnemyTeam& enemy = world.enemy_team();

	   // can shoot directly
	   if (enemy_can_shoot_goal(world, robot)) {
	   return 0;
	   }

	   int passes = 5;

	   for (size_t i = 0; i < enemy.size(); ++i) {
	   Robot::Ptr passee = enemy.get(i);
	   if (passee == robot) continue;
	   if (!enemy_can_pass(world, robot, passee)) continue;
	   if (enemy_can_shoot_goal(world, passee)) {
	   passes = std::min(passes, 1);
	   continue;
	   }

	   for (std::size_t j = 0; j < enemy.size(); ++j) {
	   Robot::Ptr next_passee = enemy.get(j);
	   if (next_passee == robot) continue;
	   if (next_passee == passee) continue;
	   if (!enemy_can_pass(world, passee, next_passee)) continue;
	   if (!enemy_can_shoot_goal(world, next_passee)) continue;
	   passes = std::min(passes, 2);
	   }
	   }

	   return passes;
	 */
}

std::vector<Evaluation::Threat> AI::HL::STP::Evaluation::calc_enemy_threat(const World &world) {
	const EnemyTeam &enemy = world.enemy_team();

	Robot::Ptr enemy_baller = calc_enemy_baller(world);

	std::vector<Evaluation::Threat> threats(enemy.size());
	for (size_t i = 0; i < enemy.size(); ++i) {
		threats[i].goal_angle = calc_enemy_best_shot_goal(world, enemy.get(i)).second;
		threats[i].can_shoot_goal = enemy_can_shoot_goal(world, enemy.get(i));
		threats[i].passes_reach = 5;
		threats[i].passes_goal = 5;
		threats[i].robot = enemy.get(i);

		if (enemy.get(i) == enemy_baller) {
			threats[i].passes_reach = 0;
		}

		if (possess_ball(world, enemy.get(i))) {
			threats[i].passes_reach = 0;
		}
		if (threats[i].can_shoot_goal) {
			threats[i].passes_goal = 0;
		}
	}

	// all-pairs shortest paths
	for (size_t k = 0; k < enemy.size(); ++k) {
		for (size_t i = 0; i < enemy.size(); ++i) {
			for (size_t j = 0; j < enemy.size(); ++j) {
				if (i == j) {
					continue;
				}
				if (!enemy_can_pass(world, enemy.get(i), enemy.get(j))) {
					continue;
				}

				// if can pass from i to j

				if (threats[j].passes_reach > threats[i].passes_reach + 1) {
					threats[j].passes_reach = threats[i].passes_reach + 1;
					threats[j].passer = enemy.get(i);
				}

				if (threats[i].passes_goal > threats[j].passes_goal + 1) {
					threats[i].passes_goal = threats[j].passes_goal + 1;
					threats[i].passee = enemy.get(j);
				}
			}
		}
	}

	return threats;
}

