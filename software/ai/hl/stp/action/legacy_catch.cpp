#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "ai/hl/stp/action/legacy_catch.h"
#include "ai/hl/stp/action/legacy_move.h"
#include "ai/hl/stp/action/legacy_action.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/action/legacy_dribble.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

bool AI::HL::STP::Action::legacy_catch_stopped_ball(World world, Player player, Point target) {
	double inner_radius = Robot::MAX_RADIUS + 0.10;
	Point proj_ball = Evaluation::baller_catch_position(world, player);
	Point behind_ball = proj_ball - (target - proj_ball).norm() * inner_radius;
	Point pp = player.position();

	double r = (proj_ball - pp).len();

	if (r > 0.3) {
		player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
		player.mp_move(behind_ball, (target - behind_ball).orientation());
		return false;
	}
	else if (r > inner_radius || (behind_ball - pp).len() > 0.02) {
			// ((proj_ball - player.position()).orientation() - (target - proj_ball).orientation()).abs() > Angle::of_degrees(20)) {
		player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::AVOID_BALL_TINY | AI::Flags::MoveFlags::CAREFUL);
		Action::move(world, player, behind_ball, (target - player.position()).orientation());
		return false;
	}
	else if (!player.has_ball()) {
		player.flags(AI::Flags::calc_flags(world.playtype()) | AI::Flags::MoveFlags::CAREFUL);
		Action::move(world, player, proj_ball, (target - player.position()).orientation());
		return false;
	}

	return true;
}

bool AI::HL::STP::Action::legacy_catch_ball(World world, Player player, Point target) {
	// player has ball, don't try to reposition
#warning this is bad
	if (player.has_ball()) {
		return true;
	}

	Point proj_ball = Evaluation::baller_catch_position(world, player);
	Point behind_ball = proj_ball - (target - proj_ball).norm() * (Robot::MAX_RADIUS + 0.2);

	if (Geom::dist(Geom::Seg(proj_ball, behind_ball), player.position()) > 0.05 ||
			(player.orientation() - (target - player.position()).orientation()).abs() > Angle::of_degrees(8)) {
		player.flags(player.flags() | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
		player.mp_move(behind_ball, (target - behind_ball).orientation());
		return false;
	}

			// !(player.position() - proj_ball).len() > Robot::MAX_RADIUS + 0.05) {

	/*
	if ((player.orientation() - (target - player.position()).orientation()).abs() > Angle::of_degrees(1) &&
		player.flags(AI::Flags::MoveFlags::AVOID_FRIENDLY_DEFENSE | AI::Flags::MoveFlags::AVOID_ENEMY_DEFENSE);
		// player.mp_dribble(proj_ball, (proj_ball - player.position()).orientation());
		Action::dribble(world, player, proj_ball, (target - player.position()).orientation());
		return false;
	}
	*/

	if (!player.has_ball()) {
		player.flags(player.flags() & ~AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
		Action::dribble(world, player, proj_ball, (target - player.position()).orientation());
		return false;
	}

	return true;
}
