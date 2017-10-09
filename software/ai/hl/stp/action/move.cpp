#include "ai/flags.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/action.h"

using namespace AI::HL::STP;

// if should_wait is false, robot stops after reaching destination
void AI::HL::STP::Action::move(caller_t& ca, World world, Player player, Point dest, bool should_wait) {
	Primitive prim = Primitives::Move(player, dest, (world.ball().position() - player.position()).orientation());
	if(should_wait) Action::wait_move(ca, player, dest);
}

void AI::HL::STP::Action::move(caller_t& ca, World, Player player, Point dest,  Angle orientation, bool should_wait) {
	Primitive prim = Primitives::Move(player, dest, orientation);
	if(should_wait) Action::wait_move(ca, player, dest, orientation);
}

void move_dribble(caller_t& ca, World world, Player player, Angle orientation, Point dest, bool should_wait) {
	Primitive prim = Primitives::Dribble(player, dest, orientation, false);
	if(should_wait) Action::wait_move(ca, player, dest, orientation);
}

void AI::HL::STP::Action::move_careful(caller_t& ca, World world, Player player, Point dest, bool should_wait) {
	player.flags(player.flags() | AI::Flags::MoveFlags::CAREFUL);

	Primitive prim = Primitives::Move(player, dest, (world.ball().position() - player.position()).orientation());
	if(should_wait) Action::wait_move(ca, player, dest);
}



