#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <algorithm>
#include <cmath>

using namespace AI::HL::STP;

namespace {
	DoubleParam alpha("Decay constant for the ball velocity", "STP/Action/shoot", 0.1, 0.0, 1.0);

	DoubleParam pass_threshold("Angle threshold (in degrees) that defines passing accuracy, smaller is more accurate", "STP/Action/shoot", 20.0, 0.0, 90.0);

	DoubleParam pass_speed("kicking speed for making a pass", "STP/Action/shoot", 7.0, 1.0, 10.0);
}

bool AI::HL::STP::Action::shoot(const World &world, Player::Ptr player) {
	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);

	if (!player->has_ball()) {
		LOG_INFO("chase pivot wtf");
		chase_pivot(world, player, shoot_data.target);
		return false;
	}

	if (shoot_data.blocked) { // still blocked, just aim
		LOG_INFO("blocked, pivot");
		pivot(world, player, shoot_data.target);
		return false;
	}

	LOG_INFO(Glib::ustring::compose("allowance %1 shoot_accuracy %2", shoot_data.accuracy_diff, Evaluation::shoot_accuracy));

	if (shoot_data.can_shoot) {
		if (!player->chicker_ready()) {
			LOG_INFO("chicker not ready");
			return false;
		}
		LOG_INFO("kick 1");
		player->autokick(10.0);
		return true;
	}

	LOG_INFO("aiming");
	pivot(world, player, shoot_data.target);
	return false;
}

bool AI::HL::STP::Action::shoot_target(const World &world, Player::Ptr player, const Point target, bool pass) {
	if (!player->has_ball()) {
		chase_pivot(world, player, target);
		return false;
	}
	if(within_angle_thresh(player, target, pass_threshold)){
		if (!player->chicker_ready()) {
			LOG_INFO("chicker not ready");
			return false;
		}
		LOG_INFO("kick 2");
		if (pass) player->autokick(pass_speed);
		else player->autokick(10.0);
		return true;
	}
	pivot(world, player, target);
	return false;
}

bool AI::HL::STP::Action::within_angle_thresh(Player::Ptr player, const Point target, double threshold){
	Point pass_dir = (target - player->position()).norm();
	Point facing_dir(1,0);
	facing_dir = facing_dir.rotate(player->orientation());
	double dir_thresh = cos( (pass_threshold*M_PI / 180.0));
	return facing_dir.dot(pass_dir) > dir_thresh;
}


#warning this is broken
bool AI::HL::STP::Action::shoot(const World &world, Player::Ptr player, const Point target, double tol, double delta) {
	player->move(target, (target - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::CATCH);
	player->prio(AI::Flags::MovePrio::HIGH);

	Point segA = player->position();
	Point segB((world.field().total_length() + world.field().total_width()), 0);
	segB = segB.rotate(player->orientation());
	double error = lineseg_point_dist(target, segA, segB);
	if (error > tol) {
		return false;
	}
	arm(world, player, target, delta);
	return player->has_ball() && player->chicker_ready();
}

double AI::HL::STP::Action::shoot_speed(double distance, double delta, double alph) {
	double a = alph;
	if(alph<0){
		a = alpha;
	}
	double speed = a * distance / (1 - std::exp(-a * delta));
	if (speed > 10.0) {
		speed = 10.0; // can't kick faster than this
	}
	if (speed < 0) {
		speed = 0; // can't kick slower than this
	}
	return speed;
}

bool AI::HL::STP::Action::arm(const World &world, Player::Ptr player, const Point target, double delta) {
	double dist_max = 10.0 * (1 - std::exp(-alpha * delta)) / alpha;
	// make the robot kick as close to the target as possible
	Point robot_dir(1, 0);
	robot_dir = robot_dir.rotate(player->orientation());
	double distance = (target - world.ball().position()).dot(robot_dir);

	if (distance > dist_max) {
		player->autokick(10.0);
		return false;
	}
	double speed = shoot_speed(distance);
	if (speed > 10.0) {
		speed = 10.0; // can't kick faster than this
	}
	if (speed < 0) {
		speed = 0; // can't kick slower than this
	}
	player->autokick(speed);
	return true;
}

