#include <algorithm>
#include <cmath>

#include "ai/flags.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "geom/rect.h"
#include "util/param.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

namespace {
	BoolParam AUTO_ORIENT(u8"Orient towards enemy goal", u8"AI/HL/STP/Action/Shoot", true);
	DoubleParam PIVOT_RADIUS(u8"Pivot_shoot radius", u8"AI/HL/STP/Action/Shoot", 0.3, 0, 1);
	DoubleParam ANGLE_THRESH(u8"Pivot_shoot angle thresh", u8"AI/HL/STP/Action/Shoot", 20, 1, 180);
	//BoolParam REPOSITION(u8"Strafe towards enemy openings", u8"AI/HL/STP/Action/Shoot", true);
	BoolParam TEAMMATE_CHECK(u8"Include teammates in strafe sweep", u8"AI/HL/STP/Action/Shoot", true);
	const double FAST = 100.0;
	DoubleParam FAST_BALL(u8"Default Shooting Speed", u8"AI/HL/STP/Shoot", 8.0, 0.0, 32.0);
}

bool AI::HL::STP::Action::shoot_goal(World world, Player player, bool use_reduced_radius) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player, use_reduced_radius);

	const Angle orient = (shoot_data.target - player.position()).orientation();

	Point behind_ball = world.ball().position() - (shoot_data.target - world.ball().position()).norm() * (Robot::MAX_RADIUS + 0.16);
	player.mp_move(behind_ball, (shoot_data.target - behind_ball).orientation());

	if ((world.ball().position() - player.position()).x < 0 || (Geom::dist(Geom::Line(behind_ball, world.ball().position()), player.position()) > 0.1)) return false;

	player.mp_shoot(world.ball().position(), orient, false, BALL_MAX_SPEED);
	
	return true;

	if (AUTO_ORIENT) {
		if (player.orientation().to_degrees() - (world.field().enemy_goal() -
				player.position()).orientation().to_degrees() > 4)
		{
			move(player, (world.field().enemy_goal() -
				player.position()).orientation(), player.position());
		}
	}

	if (shoot_data.can_shoot) {
		if (!player.chicker_ready()) {
			LOG_INFO(u8"chicker not ready");
			// angle is right but chicker not ready, ram the ball and get closer
			// to target, only use in normal play
			if (world.playtype() == AI::Common::PlayType::PLAY) {
				ram(world, player, shoot_data.target);
			}
			return false;
		}
		LOG_INFO(u8"autokick");
		intercept(player, shoot_data.target);
		player.mp_shoot(player.position(), player.orientation(), false, BALL_MAX_SPEED);
		return true;
	}

	return false;
}

bool AI::HL::STP::Action::shoot_target(World world, Player player,
	const Point target, double velocity)
{
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	intercept(player, target);

	if (!Evaluation::player_within_angle_thresh(player, target, passer_angle_threshold)) {
		return false;
	}

	// angle is right but chicker not ready, ram the ball and get closer to target
	if (!player.chicker_ready()) {
		LOG_INFO(u8"chicker not ready");
		// angle is right but chicker not ready, ram the ball and get closer to
		// target, only use in normal play
		if (world.playtype() == AI::Common::PlayType::PLAY) {
			ram(world, player, target);
		}
		return false;
	}

	LOG_INFO(u8"autokick");
	player.mp_shoot(player.position(), player.orientation(), false, velocity);
	return true;
}

bool AI::HL::STP::Action::shoot_pass(World world, Player shooter, Player target) {
	// Avoid defense areas
	shooter.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	return shoot_pass(world, shooter, target.position());
}

bool AI::HL::STP::Action::shoot_pass(World world, Player player, const Point target) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	return shoot_pass(world, player, target, passer_angle_threshold);
}

bool AI::HL::STP::Action::shoot_pass(World world, Player player, const Point target,
	Angle angle_tol)
{
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	// checker shooter orientation
	if (!Evaluation::player_within_angle_thresh(player, target, angle_tol)) {
		return false;
	}

	if (player.has_ball() && !player.chicker_ready()) {
		LOG_INFO(u8"chicker not ready");
		return false;
	}

	// check receiver is within passing range & angle
	double distance_tol = (target - player.position()).len() * angle_tol.sin() +
		AI::HL::STP::Action::target_region_param;
	bool ok = false;

	for (const Player i : world.friendly_team()) {
		if (i != player) {
			bool curr_ok = (target - i.position()).len() < distance_tol &&
				Evaluation::passee_facing_passer(player, i);
			ok = ok || curr_ok;
		}
	}

	if (ok) {
		player.mp_shoot(player.position(), player.orientation(), false, BALL_MAX_SPEED); //shooting at max speed?
		return true;
	}

	return false;
}

void AI::HL::STP::Action::pivot_shoot(World world, Player player, const Point target,
	double velocity)
{
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	Point target2ball = world.ball().position() - target;
	player.mp_shoot(world.ball().position(), (-target2ball).orientation(), false, velocity);
}
