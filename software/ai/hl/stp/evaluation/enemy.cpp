#include "ai/hl/stp/evaluation/enemy.h"
#include "geom/util.h"
#include "ai/hl/util.h"
#include "geom/angle.h"

#include <algorithm>

using namespace AI::HL::W;
using namespace AI::HL::Util;
using namespace AI::HL::STP;

namespace {
}

std::pair<Point, double> Evaluation::calc_enemy_best_shot(const Field &f, const std::vector<Point> &obstacles, const Point &p, const double radius) {
	const Point p1 = Point(-f.length() / 2.0, -f.goal_width() / 2.0);
	const Point p2 = Point(-f.length() / 2.0, f.goal_width() / 2.0);
	return angle_sweep_circles(p, p1, p2, obstacles, radius * Robot::MAX_RADIUS);
}

std::pair<Point, double> Evaluation::calc_enemy_best_shot(const World &world, const Robot::Ptr enemy, const double radius) {
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
	return calc_enemy_best_shot(world.field(), obstacles, enemy->position(), radius);
}

namespace {
	std::pair<Point, double> calc_enemy_best_shot_target(const World &world, const Point &target_pos, const Robot::Ptr enemy, const double radius = 1.0) {
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

		return AI::HL::Util::calc_best_shot_target(target_pos, obstacles, enemy->position(), radius);
	}

	bool enemy_can_receive(const World &world, const Robot::Ptr enemy) {
		const Ball &ball = world.ball();
		if ((ball.position() - enemy->position()).lensq() < POS_CLOSE) {
			return true;
		}
		// if the enemy is not facing the ball, forget it
		const Point ray = ball.position() - enemy->position();
		if (angle_diff(ray.orientation(), enemy->orientation()) > ORI_PASS_CLOSE) {
			return false;
		}

		const Point direction = ray.norm();
		const double distance = (ball.position() - enemy->position()).len();
		const FriendlyTeam &friendly = world.friendly_team();
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			const Player::CPtr plr = friendly.get(i);
			const Point pp = plr->position() - enemy->position();
			const double proj = pp.dot(direction);
			const double perp = sqrt(pp.dot(pp) - proj * proj);
			if (proj <= 0) {
				continue;
			}
			if (proj < distance && perp < shoot_accuracy + Robot::MAX_RADIUS + Ball::RADIUS) {
				return false;
			}
		}
		const EnemyTeam &enemies = world.enemy_team();
		for (std::size_t i = 0; i < enemies.size(); ++i) {
			const Robot::Ptr rob = enemies.get(i);
			if (posses_ball(world, rob) || rob == enemy) {
				continue;
			}
			const Point rp = rob->position() - enemy->position();
			const double proj = rp.dot(direction);
			const double perp = sqrt(rp.dot(rp) - proj * proj);
			if (proj <= 0) {
				continue;
			}
			if (proj < distance && perp < shoot_accuracy + Robot::MAX_RADIUS + Ball::RADIUS) {
				return false;
			}
		}
		return true;
	}
}

bool available_enemy_pass(const World &world, const Robot::Ptr passer, const Robot::Ptr passee) {
	// assuming a good enemy team
	double minangle = 5;
	double maxdist = world.field().length() / 2;

	if (!enemy_can_receive(world, passee)) {
		return false;
	}

	const double dist = (passee->position() - world.ball().position()).len();
	const double angle = calc_enemy_best_shot_target(world, passee->position(), passer).second;

	return angle >= minangle && dist < maxdist;
}

std::vector<Robot::Ptr> get_passees(const World& world, Robot::Ptr robot) {
	const EnemyTeam& enemy = world.enemy_team();

	// don't count this robot
	std::vector<Robot::Ptr> passees;
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (robot == enemy.get(i)) continue;
		if (available_enemy_pass(world, robot, enemy.get(i))) {
			passees.push_back(enemy.get(i));
		}
	}
	return passees;
}

int AI::HL::STP::Evaluation::calc_enemy_pass(const World &world, const Robot::Ptr robot) {

	bool blocked = enemy_can_receive(world, robot);

	std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

	// don't count this robot
	std::vector<Robot::Ptr> passees;
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		if (robot == enemies[i]) continue;
		if (available_enemy_pass(world, robot, enemies[i])) {
			passees.push_back(enemies[i]);
		}
	}

	int passes = -1;

	if (blocked || passees.size() == 0) {
		passes = 5;
	} else if (calc_enemy_best_shot_target(world, world.field().friendly_goal(), robot, Robot::MAX_RADIUS).second > shoot_accuracy) {
		passes = 0;
	}
	for (std::size_t i = 0; i < passees.size(); ++i) {
		if (calc_enemy_best_shot_target(world, world.field().friendly_goal(), passees[i], Robot::MAX_RADIUS).second > shoot_accuracy) {
			passes = 1;
		} else {
			std::vector<Robot::Ptr> next_passees;
			// don't count that robot
			for (std::size_t i = 0; i < enemies.size(); ++i) {
				if (enemies[i] == passees[i]) continue;
				if (available_enemy_pass(world, robot, enemies[i])) {
					next_passees.push_back(enemies[i]);
				}
			}

			for (std::size_t j = 0; j < next_passees.size(); ++j) {
				if (calc_enemy_best_shot_target(world, world.field().friendly_goal(), next_passees[j], Robot::MAX_RADIUS).second > shoot_accuracy) {
					passes = 2;
				}
			}
		}
	}

	return passes;
}

std::vector<Evaluation::Threat> AI::HL::STP::Evaluation::calc_enemy_threat(const World &world) {
	const EnemyTeam& enemy = world.enemy_team();

	std::vector<Evaluation::Threat> threats(enemy.size());
	for (size_t i = 0; i < enemy.size(); ++i) {
		threats[i].goal_angle = calc_enemy_best_shot(world, enemy.get(i)).second;
	}

	return threats;
}

