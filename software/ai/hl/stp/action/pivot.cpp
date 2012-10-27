#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include <cmath>
#include <algorithm>

void AI::HL::STP::Action::intercept_pivot(AI::HL::STP::World world, AI::HL::STP::Player player, const Point target) {
	if (Evaluation::ball_in_pivot_thresh(world, player)) {
		pivot(world, player, target);
	} else {
		intercept(player, target);
	}
}

void AI::HL::STP::Action::pivot(AI::HL::STP::World world, AI::HL::STP::Player player, const Point target, const double radius) {
	const Angle ori = (target - player.position()).orientation();
	// set the destination point to be just behind the ball in the correct direction at the offset distance
	Point dest = -(target - world.ball().position()).norm() * radius + world.ball().position();

	player.move(dest, ori, Point());
	player.type(AI::Flags::MoveType::PIVOT);
	player.prio(AI::Flags::MovePrio::HIGH);
}

