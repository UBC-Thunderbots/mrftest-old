#include "ai/hl/stp/evaluation/ball.h"
#include "ai/util.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/param.h"

#include <set>

using namespace AI::HL::STP;

namespace {

	BoolParam smart_possess_ball("Smart possess ball (instead of has ball only)", "STP/ball", true);

	DoubleParam enemy_pivot_threshold("circle radius in front of enemy robot to consider possesion (meters)", "STP/ball", 0.1, 0.0, 1.0);

	// Player::CPtr baller;
}

DoubleParam Evaluation::pivot_threshold("circle radius in front of robot to enable pivot (meters)", "STP/ball", 0.1, 0.0, 1.0);

bool Evaluation::ball_in_pivot_thresh(const World &world, Player::CPtr player) {
	Point unit_vector = Point::of_angle(player->orientation());
	Point circle_center = player->position() + Robot::MAX_RADIUS * unit_vector;
	double dist = (circle_center - world.ball().position()).len();
	return dist < pivot_threshold;
}

bool Evaluation::possess_ball(const World &world, Player::CPtr player) {
	if (player->has_ball()) {
		return true;
	}
	if (!smart_possess_ball) {
		return false;
	}
	return ball_in_pivot_thresh(world, player);
}

bool Evaluation::possess_ball(const World &world, Robot::Ptr robot) {
	// true if in pivot thresh
	Point unit_vector = Point::of_angle(robot->orientation());
	Point circle_center = robot->position() + Robot::MAX_RADIUS * unit_vector;
	double dist = (circle_center - world.ball().position()).len();
	return dist < enemy_pivot_threshold;
}

Player::CPtr Evaluation::calc_friendly_baller(const World &world) {
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (possess_ball(world, friendly.get(i))) {
			return friendly.get(i);
		}
	}
	return Player::CPtr();
}

Robot::Ptr Evaluation::calc_enemy_baller(const World &world) {
	const EnemyTeam &enemy = world.enemy_team();
	Robot::Ptr robot;
	double best_dist = 1e99;
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (possess_ball(world, enemy.get(i))) {
			return enemy.get(i);
		}
		Point dest;
		AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), enemy.get(i)->position(), dest);
		double dist = (enemy.get(i)->position() - dest).len();
		if (!robot.is() || dist < best_dist) {
			best_dist = dist;
			robot = enemy.get(i);
		}
	}
	return robot;
}

Point Evaluation::calc_fastest_grab_ball_dest(const World &world, Player::Ptr player){
	Point dest;
	AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), player->position(), dest);
	return dest;
}

