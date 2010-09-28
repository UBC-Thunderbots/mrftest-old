#include "ai/hl/tactics.h"
#include "ai/hl/util.h"

using namespace AI::HL::W;

namespace {
	DoubleParam shoot_accuracy("Shooting Accuracy (degrees)", 5.0, 0.1, 10.0);
}

void AI::HL::Tactics::shoot(World &world, Player::Ptr player, const bool force) {
	if (player->has_ball()) {
		std::pair<Point, double> target = AI::HL::Util::calc_best_shot(world, player);
#warning incomplete, waiting for move API
		if (target.second == 0) { // blocked
			if (force) {
			}
		} else {
			// call the other shoot function with the specified target
		}
	} else {
#warning incomplete, waiting for move API
		// chase ball
		// TODO: set the flag
		player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	}
}

