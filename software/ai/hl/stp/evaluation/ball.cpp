#include "ai/hl/stp/evaluation/ball.h"
#include "ai/util.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/param.h"

#include <set>

using namespace AI::HL::STP;

namespace {

	BoolParam smart_possess_ball("Smart possess ball (instead of has ball only)", "STP/ball", true);

	DoubleParam enemy_pivot_threshold("circle radius in front of enemy robot to consider possesion (meters)", "STP/ball", 0.1, 0.0, 1.0);
}

DoubleParam AI::HL::STP::Evaluation::pivot_threshold("circle radius in front of robot to enable pivot (meters)", "STP/ball", 0.1, 0.0, 1.0);

bool AI::HL::STP::Evaluation::ball_in_pivot_thresh(const World &world, Player::CPtr player) {
	Point unit_vector = Point::of_angle(player->orientation());
	Point circle_center = player->position() + Robot::MAX_RADIUS * unit_vector;
	double dist = (circle_center - world.ball().position()).len();
	return dist < pivot_threshold;
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

bool AI::HL::STP::Evaluation::possess_ball(const World &world, Robot::Ptr robot) {
	// true if in pivot thresh
	Point unit_vector = Point::of_angle(robot->orientation());
	Point circle_center = robot->position() + Robot::MAX_RADIUS * unit_vector;
	double dist = (circle_center - world.ball().position()).len();
	return dist < enemy_pivot_threshold;
}

Player::CPtr AI::HL::STP::Evaluation::calc_friendly_baller(const World &world) {
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (possess_ball(world, friendly.get(i))) {
			return friendly.get(i);
		}
	}
	return Player::CPtr();
}

Robot::Ptr AI::HL::STP::Evaluation::calc_enemy_baller(const World &world) {
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (possess_ball(world, enemy.get(i))) {
			return enemy.get(i);
		}
	}
	return Robot::Ptr();
}

Point AI::HL::STP::Evaluation::grab_ball(const World &world, Player::Ptr player){
	Point dest;
	AI::Util::grab_ball_dest(world.ball().position(), world.ball().velocity(), player->position(), dest);
	return dest;
}

bool AI::HL::STP::Evaluation::can_pass(const World& world, Player::CPtr passer, Player::CPtr passee) {
	
#warning use better method
	std::vector<Point> enemy_pos;
	const EnemyTeam& enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}
	const FriendlyTeam& friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (friendly.get(i) == passer) continue;
		if (friendly.get(i) == passee) continue;
		enemy_pos.push_back(friendly.get(i)->position());
	}
	return AI::HL::Util::path_check(passer->position(), passee->position(), enemy_pos, Robot::MAX_RADIUS * pass_width);
}

bool AI::HL::STP::Evaluation::can_pass(const World& world, const Point p1, const Point p2) {

#warning use better method
	std::vector<Point> enemy_pos;
	const EnemyTeam& enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}
	return AI::HL::Util::path_check(p1, p2, enemy_pos, Robot::MAX_RADIUS * pass_width);
}

