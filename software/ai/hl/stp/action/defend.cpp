#include "ai/hl/stp/action/defend.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/flags.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam repel_dist("Distance the defender should repel the ball in robot radius", "STP/Action/defend", 3.0, 2.0, 6.0);
}

void AI::HL::STP::Action::defender_move(World world, Player player, const Point dest) {
	// if the ball is too close we repel or chip
	if ((world.ball().position() - player.position()).len() < repel_dist * Robot::MAX_RADIUS) {
		if ((world.ball().position() - world.field().friendly_goal()).len() < repel_dist * 3) {
			chip_target(world, player, Point(0,0));
		} else {
			corner_repel(world, player);
		}
		return;
	}
	player.move(dest, (world.ball().position() - player.position()).orientation(), Point());
	player.type(AI::Flags::MoveType::NORMAL);
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE);
}

