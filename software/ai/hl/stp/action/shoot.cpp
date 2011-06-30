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
#include "ai/hl/stp/param.h"
using namespace AI::HL::STP;

namespace {

}

void AI::HL::STP::Action::autokick(Player::Ptr player, const Point target, double velocity) {
	if (player->kicker_directional()) {
		double angle = angle_diff(player->orientation(), (target - player->position()).orientation());
		player->autokick(velocity, angle);
	} else {
		player->autokick(velocity);
	}
}

bool AI::HL::STP::Action::shoot_goal(const World &world, Player::Ptr player) {
	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);

	chase_pivot(world, player, shoot_data.target);

	if (shoot_data.blocked) { // still blocked, just aim
		return false;
	}

	// LOG_INFO(Glib::ustring::compose("allowance %1 shoot_accuracy %2", shoot_data.accuracy_diff, Evaluation::shoot_accuracy));

	if (shoot_data.can_shoot) {
		if (!player->chicker_ready()) {
			LOG_INFO("chicker not ready");
			return false;
		}
		LOG_INFO("autokick");
		autokick(player, shoot_data.target, 10.0);
		return true;
	}

	return false;
}

bool AI::HL::STP::Action::shoot_target(const World &world, Player::Ptr player, const Point target, double velocity) {
	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot_target(world, player, target);
	chase_pivot(world, player, target);

	if(shoot_data.can_shoot/*within_angle_thresh(player, target, pass_threshold)*/) {
		if (!player->chicker_ready()) {
			LOG_INFO("chicker not ready");
			return false;
		}
		LOG_INFO("autokick");
		autokick(player, target, velocity);
		return true;
	}
	return false;
}

bool AI::HL::STP::Action::shoot_pass(const World &world, Player::Ptr player, const Point target) {
	return shoot_pass(world, player, target, pass_threshold);
}

bool AI::HL::STP::Action::shoot_pass(const World &world, Player::Ptr player, const Point target, double angle_tol) {

	chase_pivot(world, player, target);
	// checker shooter orientation
	if (!within_angle_thresh(player, target, angle_tol)) {
		return false;
	}
	autokick(player, target, pass_speed);
	
	// check receiver orientation
//	Player::CPtr receiver = Evaluation::nearest_friendly(world, target);
//	if (!within_angle_thresh(receiver, player->position(), pass_threshold)) {
//		return false;
//	}

	if (player->has_ball() && within_angle_thresh(player, target, angle_tol) && !player->chicker_ready()) {
		LOG_INFO("chicker not ready");
		return false;
	}
	
	// only partly accurate whether an autokick or not
	return true;
}

#warning TODO REFACTOR, this should also be somewhere in evaluation
bool AI::HL::STP::Action::within_angle_thresh(Player::CPtr player, const Point target, double threshold){
	return within_angle_thresh(player->position(), player->orientation(), target, threshold);
}

bool AI::HL::STP::Action::within_angle_thresh(const Point position, const double orientation, const Point target, double threshold) {
	Point pass_dir = (target - position).norm();
	Point facing_dir = Point(1,0).rotate(orientation);
	double dir_thresh = cos((threshold*M_PI / 180.0));
	return facing_dir.dot(pass_dir) > dir_thresh;
}

double AI::HL::STP::Action::shoot_speed(double distance, double delta, double alph) {
	double a = alph;
	if(alph<0) a = alpha;
	
	double speed = a * distance / (1 - std::exp(-a * delta));
	if (speed > 10.0) speed = 10.0; // can't kick faster than this
	if (speed < 0) speed = 0; // can't kick slower than this, this value in reality should be somwhere > 4
	
	return speed;
}

