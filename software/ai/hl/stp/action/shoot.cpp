#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/team.h"
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

bool AI::HL::STP::Action::shoot_goal(const World &world, Player::Ptr player) {
	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);

	if (!player->has_ball()) {
		chase_pivot(world, player, shoot_data.target);
		return false;
	}

	if (shoot_data.blocked) { // still blocked, just aim
		chase_pivot(world, player, shoot_data.target);
		return false;
	}

	// LOG_INFO(Glib::ustring::compose("allowance %1 shoot_accuracy %2", shoot_data.accuracy_diff, Evaluation::shoot_accuracy));

	pivot(world, player, shoot_data.target);
	if (shoot_data.can_shoot) {
		if (!player->chicker_ready()) {
			LOG_INFO("chicker not ready");
			return false;
		}
		LOG_INFO("autokick");
		player->autokick(10.0);
		return true;
	}

	return false;
}

bool AI::HL::STP::Action::shoot_target(const World &world, Player::Ptr player, const Point target, bool pass) {
	if (!player->has_ball()) {
		chase_pivot(world, player, target);
		return false;
	}

	pivot(world, player, target);
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
	return false;
}

bool AI::HL::STP::Action::shoot_pass(const World &world, Player::Ptr player, const Point target) {
	if (!player->has_ball()) {
		chase_pivot(world, player, target);
		return false;
	}

	pivot(world, player, target);

	// checker shooter orientation
	if (!within_angle_thresh(player, target, pass_threshold)) {
		return false;
	}

	// check receiver orientation
	Player::CPtr receiver = Evaluation::nearest_friendly(world, target);
	if (!within_angle_thresh(receiver, player->position(), pass_threshold)) {
		return false;
	}

	if (!player->chicker_ready()) {
		LOG_INFO("chicker not ready");
		return false;
	}

	LOG_INFO("kick");
	player->autokick(pass_speed);

	return true;
}

#warning REFACTOR
bool AI::HL::STP::Action::within_angle_thresh(Player::CPtr player, const Point target, double threshold){
	Point pass_dir = (target - player->position()).norm();
	Point facing_dir(1,0);
	facing_dir = facing_dir.rotate(player->orientation());
	double dir_thresh = cos( (pass_threshold*M_PI / 180.0));
	return facing_dir.dot(pass_dir) > dir_thresh;
}

bool AI::HL::STP::Action::shoot_region(const World &world, Player::Ptr player, const Point target, double radius, double delta) {
	if (!player->has_ball()) {
		chase_pivot(world, player, target);
		return false;
	}

	pivot(world, player, target);

	const double dist = (player->position() - target).len();
	const double theta = std::atan(radius / dist);

	double ori = (target - player->position()).orientation();
	double ori_diff = angle_diff(ori, player->orientation());
	double accuracy_diff = radians2degrees(ori_diff - (theta / 2));

	if (accuracy_diff > -Evaluation::shoot_accuracy) {
		return false;
	}
	if (!arm(world, player, target, delta)) {
		return false;
	}
	return player->chicker_ready();
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

