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

	//bool enemy_blocked(const World &world, const Robot::Ptr enemy){}
	

	#warning TODO: maybe the source to a point instead of defaulting to ball.
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

EnemyThreat eval_enemy(const World &world, Robot::Ptr robot) {

	#warning Under Construction
	EnemyThreat enemy_threat;

	enemy_threat.threat_dist = std::min((robot->position() - world.ball().position()).len() , (robot->position() - world.field().friendly_goal()).len());
	
	std::vector<AI::HL::W::Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
	
	std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(robot->position()));

	// don't count this robot
	for (std::size_t i = 1; i < enemies.size(); ++i) {
		if (enemy_can_receive(world, enemies[i])){
			enemy_threat.pass_enemies.push_back(enemies[i]);
		} 
	}	
	

	return enemy_threat;
}



