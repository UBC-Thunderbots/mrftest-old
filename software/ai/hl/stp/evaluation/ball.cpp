#include "ai/hl/stp/evaluation/ball.h"
#include "util/param.h"
#include "ai/hl/util.h"

#include <set>

using namespace AI::HL::STP;

namespace {
	DoubleParam posses_threshold("circle radius in front of robot to consider possesion (meters)", "STP/ball", 0.1, 0.0, 1.0);

	BoolParam smart_possess_ball("Smart possess ball (instead of has ball only)", "STP/predicates", true);
}

bool AI::HL::STP::Evaluation::ball_in_pivot_thresh(const World &world, Player::CPtr player) {
	Point unit_vector = Point::of_angle(player->orientation());
	Point circle_center = player->position() + Robot::MAX_RADIUS * unit_vector;
	double dist = (circle_center - world.ball().position()).len();
	return dist < posses_threshold;
}

bool AI::HL::STP::Evaluation::possess_ball(const World &world, Player::CPtr player) {
	if (player->has_ball()) {
		return true;
	}
	if (!smart_possess_ball) {
		return false;
	}
	return ball_in_pivot_thresh(world, player);
}

Player::CPtr AI::HL::STP::Evaluation::calc_friendly_baller(const World &world) {
	const FriendlyTeam &friendly = world.friendly_team();
	std::set<Player::CPtr> players;
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (possess_ball(world, friendly.get(i))) {
			return friendly.get(i);
		}
	}
	return Player::CPtr();
}

Robot::Ptr AI::HL::STP::Evaluation::calc_enemy_baller(const World &world) {
	const EnemyTeam &enemy = world.enemy_team();
	std::set<Robot::Ptr> enemies;
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (AI::HL::Util::posses_ball(world, enemy.get(i))) {
			return enemy.get(i);
		}
	}
	return Robot::Ptr();
}

