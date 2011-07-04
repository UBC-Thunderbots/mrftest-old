#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/param.h"
#include "geom/angle.h"

using namespace AI::HL::STP;

namespace {

	DoubleParam friendly_pass_width("Friendly pass checking width (robot radius)", "STP/offense", 2, 0, 9);

	DoubleParam enemy_pass_width("Enemy pass checking width (robot radius)", "STP/enemy", 2, 0, 9);

	bool can_pass_check(const Point p1, const Point p2, const std::vector<Point>& obstacles, double tol) {

		// auto allowance = AI::HL::Util::calc_best_shot_target(passer->position(), obstacles, passee->position(), 1).second;
		// return allowance > degrees2radians(enemy_shoot_accuracy);

		// OLD method is TRIED and TESTED
		return AI::HL::Util::path_check(p1, p2, obstacles, Robot::MAX_RADIUS * tol);
	}
}

bool AI::HL::STP::Evaluation::enemy_can_pass(const World &world, const Robot::Ptr passer, const Robot::Ptr passee) {
	std::vector<Point> obstacles;
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		obstacles.push_back(friendly.get(i)->position());
	}

	return can_pass_check(passer->position(), passee->position(), obstacles, enemy_pass_width);
}

bool Evaluation::can_pass(const World& world, Player::CPtr passer, Player::CPtr passee) {

	std::vector<Point> obstacles;
	const EnemyTeam& enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}
	const FriendlyTeam& friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (friendly.get(i) == passer) continue;
		if (friendly.get(i) == passee) continue;
		obstacles.push_back(friendly.get(i)->position());
	}

	return can_pass_check(passer->position(), passee->position(), obstacles, friendly_pass_width);
}

bool Evaluation::can_pass(const World& world, const Point p1, const Point p2) {
	std::vector<Point> obstacles;
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}

	return can_pass_check(p1, p2, obstacles, friendly_pass_width);
}

bool Evaluation::passee_facing_ball(const World& world, Player::CPtr passee) {
	return player_within_angle_thresh(passee, world.ball().position(), passee_angle_threshold);
}

bool Evaluation::passee_facing_passer(Player::CPtr passer, Player::CPtr passee) {
	return player_within_angle_thresh(passee, passer->position(), passee_angle_threshold);
}

bool Evaluation::passee_suitable(const World& world, Player::CPtr passee) {

	// can't pass backwards
	if (passee->position().x < world.ball().position().x) {
		return false;
	}

	// must be at least some distance
	if ((passee->position() - world.ball().position()).len() < min_pass_dist) {
		return false;
	}

	// must be able to pass
	if (!Evaluation::can_pass(world, world.ball().position(), passee->position())) {
		return false;
	}

	if (!Evaluation::passee_facing_ball(world, passee)) {
		return false;
	}

	return true;
}

Player::CPtr Evaluation::find_random_passee(const World& world) {
	const FriendlyTeam& friendly = world.friendly_team();
	std::vector<Player::CPtr> candidates;
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (possess_ball(world, friendly.get(i))) {
			continue;
		}
		if (passee_suitable(world, friendly.get(i))) {
			candidates.push_back(friendly.get(i));
		}
	}
	if (candidates.size() == 0) {
		return Player::CPtr();
	}
	random_shuffle(candidates.begin(), candidates.end());
	return candidates.front();
}

