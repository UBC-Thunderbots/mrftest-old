#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/ram.h"
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
	const double FAST = 100.0;
}

bool AI::HL::STP::Action::shoot_goal(World world, Player::Ptr player, bool use_reduced_radius) {
	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player, use_reduced_radius);

	Player::CPtr pc = player;
	if (!Evaluation::possess_ball(world, pc)) {
		intercept_pivot(world, player, shoot_data.target);
		return false;
	}

	if (shoot_data.can_shoot) {
		if (!player->chicker_ready()) {
			LOG_INFO("chicker not ready");
			// angle is right but chicker not ready, ram the ball and get closer to target, only use in normal play
			if (world.playtype() == AI::Common::PlayType::PLAY) {
				const Point diff = world.ball().position() - player->position();
				ram(world, player, shoot_data.target, diff * FAST);
			}
			return false;
		}
		LOG_INFO("autokick");
		intercept(player, shoot_data.target);
		player->autokick(BALL_MAX_SPEED);
		return true;
	} else {
		intercept_pivot(world, player, shoot_data.target);
	}

	return false;
}

bool AI::HL::STP::Action::shoot_target(World world, Player::Ptr player, const Point target, double velocity) {
	// Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot_target(world, player, target);
	intercept_pivot(world, player, target);

	// if (shoot_data.can_shoot) {
	if (!Evaluation::player_within_angle_thresh(player, target, passer_angle_threshold)) {
		return false;
	}

	// angle is right but chicker not ready, ram the ball and get closer to target
	if (!player->chicker_ready()) {
		LOG_INFO("chicker not ready");
		// angle is right but chicker not ready, ram the ball and get closer to target, only use in normal play
		if (world.playtype() == AI::Common::PlayType::PLAY) {
			const Point diff = world.ball().position() - player->position();
			ram(world, player, target, diff * FAST);
		}
		return false;
	}

	LOG_INFO("autokick");
	player->autokick(velocity);
	return true;
}

bool AI::HL::STP::Action::shoot_pass(World world, Player::Ptr shooter, Player::CPtr target) {
	return shoot_pass(world, shooter, target->position());
}

bool AI::HL::STP::Action::shoot_pass(World world, Player::Ptr player, const Point target) {
	return shoot_pass(world, player, target, passer_angle_threshold);
}

bool AI::HL::STP::Action::shoot_pass(World world, Player::Ptr player, const Point target, Angle angle_tol) {
	intercept_pivot(world, player, target);

	// checker shooter orientation
	if (!Evaluation::player_within_angle_thresh(player, target, angle_tol)) {
		return false;
	}

	if (player->has_ball() && !player->chicker_ready()) {
		LOG_INFO("chicker not ready");
		return false;
	}

	// check receiver is within passing range & angle
	double distance_tol = (target - player->position()).len() * angle_tol.sin() + AI::HL::STP::Action::target_region_param;
	bool ok = false;

	for (std::size_t i = 0; i < world.friendly_team().size(); i++) {
		Player::CPtr p = player;
		if (world.friendly_team().get(i) != p) {
			bool curr_ok = (target - world.friendly_team().get(i)->position()).len() < distance_tol
			               && Evaluation::passee_facing_passer(player, world.friendly_team().get(i));
			ok = ok || curr_ok;
		}
	}

	if (ok) {
		player->autokick(pass_speed);
		return true;
	}

	return false;
}

