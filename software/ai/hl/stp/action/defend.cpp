#include "ai/hl/stp/action/defend.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/flags.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam repel_dist("Distance the defender should repel the ball in robot radius", "STP/Action/defend", 4.0, 1.0, 6.0);
}

void AI::HL::STP::Action::defender_move(World world, Player::Ptr player, const Point dest) {
	// if the ball is too close we repel
	if ((world.ball().position() - player->position()).len() < repel_dist * Robot::MAX_RADIUS) {
		corner_repel(world, player);
		return;
	}
	player->move(dest, (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);
}

