#include "ai/flags.h"
#include "ai/hl/stp/action/move.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::move(caller_t& ca, World world, Player player, Point dest) {
	Primitive prim = Primitives::Move(player, dest, (world.ball().position() - player.position()).orientation());
	Action::wait(ca, prim);
}

void AI::HL::STP::Action::move(caller_t& ca, World, Player player, Angle orientation, Point dest) {
	Primitive prim = Primitives::Move(player, dest, orientation);
	Action::wait(ca, prim);
}

void AI::HL::STP::Action::move_dribble(caller_t& ca, World, Player player, Angle orientation, Point dest) {
	Primitive prim = Primitives::Dribble(player, dest, orientation, false);
	Action::wait(ca, prim);
}

void AI::HL::STP::Action::move_careful(caller_t& ca, World world, Player player, Point dest) {
	player.flags(player.flags() | AI::Flags::MoveFlags::CAREFUL);

	Primitive prim = Primitives::Move(player, dest, (world.ball().position() - player.position()).orientation());
	Action::wait(ca, prim);
}

