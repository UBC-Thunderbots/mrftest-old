#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/player.h"
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
	// Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot_target(world, player, target);
	chase_pivot(world, player, target);

	//if (shoot_data.can_shoot) {
	if (Evaluation::player_within_angle_thresh(player, target, passer_angle_threshold)) {
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

bool AI::HL::STP::Action::shoot_pass(const World& world, Player::Ptr shooter, Player::CPtr target) {
#warning broken
	return shoot_pass(world, shooter, target->position());
}

bool AI::HL::STP::Action::shoot_pass(const World &world, Player::Ptr player, const Point target) {
	return shoot_pass(world, player, target, passer_angle_threshold);
}

bool AI::HL::STP::Action::shoot_pass(const World &world, Player::Ptr player, const Point target, double angle_tol) {

	chase_pivot(world, player, target);
	// checker shooter orientation
	if (!Evaluation::player_within_angle_thresh(player, target, angle_tol)) {
		return false;
	}

	if (player->has_ball() && Evaluation::player_within_angle_thresh(player, target, angle_tol) && !player->chicker_ready()) {
		LOG_INFO("chicker not ready");
		return false;
	}

	// check receiver is within passing range & angle
	double distance_tol = (target-player->position()).len()*sin(degrees2radians(angle_tol)) + AI::HL::STP::Action::target_region_param;
	bool ok=false;

	for (std::size_t i = 0; i < world.friendly_team().size(); i++) {
		Player::CPtr p = player;
		if(world.friendly_team().get(i) != p) {
			bool curr_ok =  (target - world.friendly_team().get(i)->position()).len() < distance_tol
				&& Evaluation::passee_facing_passer(player, world.friendly_team().get(i));
			ok = ok || curr_ok;
		}
	}

	if(ok) {
		autokick(player, target, pass_speed);
		return true;
	}

	return false;
}

double AI::HL::STP::Action::shoot_speed(double distance, double delta, double alph) {
	double a = alph;
	if(alph<0) a = alpha;

	double speed = a * distance / (1 - std::exp(-a * delta));
	if (speed > 10.0) speed = 10.0; // can't kick faster than this
	if (speed < 0) speed = 0; // can't kick slower than this, this value in reality should be somwhere > 4

	return speed;
}

