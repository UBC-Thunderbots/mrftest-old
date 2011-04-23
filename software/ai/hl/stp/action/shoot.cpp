#include "ai/hl/stp/action/shoot.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/actions.h"
#include "geom/util.h"
#include "util/dprint.h"
#include <cmath>
#include <algorithm>

DoubleParam alpha("Decay constant for the ball velocity", "STP/Action/shoot", 0.1, 0.0, 1.0);

bool AI::HL::STP::Action::shoot(const World &world, Player::Ptr player) {

	// TODO:
	// take into account that the optimal solution may not always be the largest opening
	std::pair<Point, double> target = AI::HL::Util::calc_best_shot(world, player);

	if (!player->has_ball()) {
		if (target.second == 0) {
			// just grab the ball, don't care about orientation
			chase(world, player);
			//LOG_INFO("chase");
		} else {
			// orient towards the enemy goal area
			//LOG_INFO("move catch");
			player->move(target.first, (world.field().enemy_goal() - player->position()).orientation(), Point());
			player->type(AI::Flags::MoveType::CATCH);
			player->prio(AI::Flags::MovePrio::HIGH);
		}
		return false;
	}

	if (target.second == 0) { // bad news, we are blocked
		Point new_target = world.field().enemy_goal();
		if (false) {
			// TODO: perhaps do a reduced radius calculation
			return shoot(world, player, new_target);
		} else { // just aim at the enemy goal
			player->move(new_target, (world.field().enemy_goal() - player->position()).orientation(), Point());
			player->type(AI::Flags::MoveType::DRIBBLE);
			player->prio(AI::Flags::MovePrio::HIGH);
		}
		return false;
	}
#warning TODO make the shoot accuracy a function of the amount of open net
	return AI::HL::STP::Action::shoot(world, player, target.first, AI::HL::Util::shoot_accuracy, 0.0);
}

bool AI::HL::STP::Action::shoot(const World &world, Player::Ptr player, const Point target, double tol, double delta) {
	player->move(target, (target - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::CATCH);
	player->prio(AI::Flags::MovePrio::HIGH);

	Point segA = player->position();
	Point segB((world.field().total_length()+world.field().total_width()),0);
	segB = segB.rotate(player->orientation());
	double error = lineseg_point_dist(target, segA, segB);
	if(error > tol){
		return false;
	}
	arm(world, player, target, delta);
	return player->has_ball() && player->chicker_ready();
}

bool AI::HL::STP::Action::arm(const World &world, Player::Ptr player, const Point target, double delta) {

	double dist_max = 10.0*(1-std::exp(-alpha*delta))/alpha;
	//make the robot kick as close to the target as possible
	Point robot_dir(1,0);
	robot_dir = robot_dir.rotate(player->orientation());
	double distance = (target - world.ball().position()).dot(robot_dir);

	if(distance > dist_max){
		player->autokick(10.0);
		return false;
	}
	double speed = alpha*distance/(1-exp(-alpha*delta));
	player->autokick(speed);
	return true;
}
