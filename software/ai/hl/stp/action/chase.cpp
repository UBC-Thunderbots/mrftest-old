#include "ai/hl/stp/action/chase.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	const double FAST = 100.0;
}

void AI::HL::STP::Action::chase(const World &world, Player::Ptr player) {
	chase(world, player, world.ball().position());
}

void AI::HL::STP::Action::chase(const World &world, Player::Ptr player, Point target) {
	player->move(world.ball().position(), (target - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::CATCH);
	player->prio(AI::Flags::MovePrio::HIGH);
}

