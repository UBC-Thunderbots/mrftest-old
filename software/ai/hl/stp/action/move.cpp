#include "ai/flags.h"
#include "ai/hl/stp/action/move.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::move(World world, Player player, const Point dest) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	player.mp_move(dest, (world.ball().position() - player.position()).orientation());
	player.type(AI::Flags::MoveType::NORMAL);
}

void AI::HL::STP::Action::move(Player player, const Angle orientation, const Point dest) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	player.mp_move(dest, orientation);
	player.type(AI::Flags::MoveType::NORMAL);
}

void AI::HL::STP::Action::move_careful(World world, Player player, const Point dest) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	player.mp_move(dest, (world.ball().position() - player.position()).orientation());
	player.flags(AI::Flags::FLAG_CAREFUL);
}

