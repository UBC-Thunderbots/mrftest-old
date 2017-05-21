#include "strafe.h"

#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/action/move.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam INCREMENT(u8"strafing increment", u8"AI/HL/STP/Action/Strafe", 0.1, 0.01, 1);
}

void Action::strafe(caller_t& ca, World world, Player player, const Point dir) {
	Action::move(ca, world, player, player.position() + (dir - player.position()).norm(INCREMENT), player.orientation());
	return;
}

void Action::strafe(caller_t& ca, World world, Player player, const Point dir, const Angle face) {
	Action::move(ca, world, player, player.position() + (dir - player.position()).norm(INCREMENT), face);
	return;
}

void Action::strafe(caller_t& ca, World world, Player player, const Point dir, const Point face) {
	Action::move(ca, world, player, player.position() + (dir - player.position()).norm(INCREMENT), (face - player.position()).orientation());
	return;
}

void Action::strafe_dribble(caller_t& ca, World world, Player player, const Point dir, const Point face) {
	Action::move(ca, world, player, player.position() + (dir - player.position()).norm(INCREMENT), (face - player.position()).orientation());
	return;
}
