#include "ai/hl/stp/action/legacy_action.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/legacy_ram.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::ram(World world, Player player, const Point dest) {
	goalie_ram(world, player, dest);
}

void AI::HL::STP::Action::ram(World world, Player player) {
	goalie_ram(world, player, world.ball().position());
}

void AI::HL::STP::Action::goalie_ram(World world, Player player, const Point dest) {
	if (player.position().orientation().to_radians() < M_PI / 2 && player.position().orientation().to_radians() > -M_PI / 2)
	{
		player.mp_shoot(dest, (world.ball().position() - player.position()).orientation(), 1.4, true);
	}
	else
	{
		player.mp_move(dest, (world.ball().position() - player.position()).orientation());
	}
}

