#include "ai/hl/stp/action/legacy_action.h"
#include "ai/hl/stp/action/legacy_pivot.h"
#include "ai/hl/stp/action/legacy_intercept.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/world.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include "ai/hl/world.h"

using namespace AI::HL::STP;
using namespace AI::HL::W;

void AI::HL::STP::Action::intercept_pivot(AI::HL::STP::World world, AI::HL::STP::Player player, const Point target) {

	Point target2ball = world.ball().position() - target;
	Point player2ball = world.ball().position() - player.position();
	Angle swing_diff = (target2ball.orientation() - player2ball.orientation() + Angle::half()).angle_mod();

	if (swing_diff.abs() <= Angle::of_degrees(20) || (player2ball.len() < 0.25 && swing_diff.abs() <= Angle::quarter())) {
		player.mp_shoot(world.ball().position(), (-target2ball).orientation(), false, BALL_MAX_SPEED);
	} else if (player2ball.len() < 0.25) {
		// Get further away so we can pivot safely.
		player.mp_move(world.ball().position() - player2ball.norm() * 0.3, (-target2ball).orientation());
	} else {
		player.mp_pivot(world.ball().position(), (-target2ball).orientation());
	}
}

void AI::HL::STP::Action::pivot(AI::HL::STP::World world, AI::HL::STP::Player player, const Point target, const double radius) {
	// set the destination point to be just behind the ball in the correct direction at the offset distance
	Point dest = -(target - world.ball().position()).norm() * radius + world.ball().position();
	player.mp_pivot(dest, (world.ball().position() - dest).orientation());
	player.prio(AI::Flags::MovePrio::HIGH);
}

