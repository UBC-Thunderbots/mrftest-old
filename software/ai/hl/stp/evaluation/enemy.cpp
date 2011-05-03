#include "ai/hl/stp/evaluation/enemy.h"
#include "geom/util.h"
#include "ai/hl/util.h"
#include "geom/angle.h"

#include <algorithm>

using namespace AI::HL::W;
using namespace AI::HL::Util;
using namespace AI::HL::STP;
using AI::HL::STP::Evaluation::EnemyThreat;

namespace {

	std::pair<Point, double> calc_enemy_best_shot(const Field &f, const std::vector<Point> &obstacles, const Point &p, const double radius) {
		const Point p1 = Point(-f.length() / 2.0, -f.goal_width() / 2.0);
		const Point p2 = Point(-f.length() / 2.0, f.goal_width() / 2.0);
		return angle_sweep_circles(p, p1, p2, obstacles, radius * Robot::MAX_RADIUS);
	}

	std::pair<Point, double> calc_enemy_best_shot(const World &world, const Robot::Ptr enemy, const double radius = 1.0) {
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

	bool available_enemy_pass(const World &world, const Robot::Ptr passer, const Robot::Ptr passee) {
		// assuming a good enemy team
		double minangle = 5;
		double maxdist = world.field().length()/2;
		
		if (!enemy_can_receive(world, passee)) {
			return false;
		}
		
		const double dist = (passee->position() - world.ball().position()).len();
		const double angle = calc_enemy_best_shot_target(world, passee->position(), passer).second;
		
		return angle >= minangle && dist < maxdist;
	}

}

EnemyThreat AI::HL::STP::Evaluation::eval_enemy(const World &world, const Robot::Ptr robot) {

	// TODO: Check for Errors
	EnemyThreat enemy_threat;

	enemy_threat.threat_dist = std::min((robot->position() - world.ball().position()).len() , (robot->position() - world.field().friendly_goal()).len());
	
	enemy_threat.blocked = enemy_can_receive(world, robot);
	
	std::vector<AI::HL::W::Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
	
	std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(robot->position()));
	
	// don't count this robot
	for (std::size_t i = 1; i < enemies.size(); ++i) {
		if (available_enemy_pass(world, robot, enemies[i])){
			enemy_threat.pass_enemies.push_back(enemies[i]);
		} 
	}	

	enemy_threat.passes = 5;
	if (enemy_threat.blocked || enemy_threat.pass_enemies.size() == 0) {
		enemy_threat.passes = 5;
	} else if (calc_enemy_best_shot_target(world, world.field().friendly_goal(), robot, Robot::MAX_RADIUS).second > shoot_accuracy){
		enemy_threat.passes = 0;
	}
	for (std::size_t i = 0; i < enemy_threat.pass_enemies.size(); ++i) {
		if (calc_enemy_best_shot_target(world, world.field().friendly_goal(), enemy_threat.pass_enemies[i], Robot::MAX_RADIUS).second > shoot_accuracy) {
			enemy_threat.passes = 1;
		}
		else {
			EnemyThreat next = eval_enemy(world, enemy_threat.pass_enemies[i]);
			for (std::size_t j = 0; j < next.pass_enemies.size(); ++j){
				if (calc_enemy_best_shot_target(world, world.field().friendly_goal(), next.pass_enemies[j], Robot::MAX_RADIUS).second > shoot_accuracy) {
				enemy_threat.passes = 2;
				}
			}	
		}
	}

	return enemy_threat;
}



