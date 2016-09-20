#include "ai/hl/stp/action/legacy_action.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/legacy_move.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::move(World world, Player player, Point dest) {
	player.mp_move(dest, (world.ball().position() - player.position()).orientation());
}

void AI::HL::STP::Action::move(World world, Player player, Point dest, Angle orientation) {
	player.mp_move(dest, orientation);
}

void AI::HL::STP::Action::move_careful(World world, Player player, Point dest) {
	player.flags(player.flags() | AI::Flags::MoveFlags::CAREFUL);
	player.mp_move(dest, (world.ball().position() - player.position()).orientation());
}

