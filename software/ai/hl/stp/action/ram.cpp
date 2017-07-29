#include "ai/flags.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/action/move.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::ram(caller_t& ca, World world, Player player, Point dest) {
	goalie_ram(ca, world, player, dest);
}

void AI::HL::STP::Action::ram(caller_t& ca, World world, Player player) {
	goalie_ram(ca, world, player, world.ball().position());
}

void AI::HL::STP::Action::goalie_ram(caller_t& ca, World world, Player player, Point dest) {
//	player.prio(AI::Flags::MovePrio::HIGH);

//	const Primitive& prim = Primitives::Move(player, dest, (world.ball().position() - player.position()).orientation());
	Action::move(ca, world, player, dest);
}

