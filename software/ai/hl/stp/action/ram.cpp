#include "ai/hl/stp/action/ram.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::ram(World world, Player player, const Point dest, const Point vel) {
	player.move(dest, (world.ball().position() - player.position()).orientation(), vel);
	player.type(AI::Flags::MoveType::NORMAL); // Should be changed back to RAM_BALL
	player.prio(AI::Flags::MovePrio::HIGH);
}

void AI::HL::STP::Action::ram(World world, Player player) {
	ram(world, player, world.ball().position(), Point());
}

