#include "ai/flags.h"
#include "ai/hl/stp/action/ram.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::ram(World world, Player player, const Point dest) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	goalie_ram(world, player, dest);
}

void AI::HL::STP::Action::ram(World world, Player player) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	goalie_ram(world, player, world.ball().position());
}

void AI::HL::STP::Action::goalie_ram(World world, Player player, const Point dest) {
	player.mp_move(dest, (world.ball().position() - player.position()).orientation());
	player.type(AI::Flags::MoveType::NORMAL); // Hopefully should not have to be changed back to RAM_BALL
	player.prio(AI::Flags::MovePrio::HIGH);
}

