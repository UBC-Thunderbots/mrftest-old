#include "ai/hl/stp/action/strafe.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam INCREMENT(u8"strafing increment", "STP/Action/Strafe", 0.1, 0.01, 1);
}

void Action::strafe(Player player, const Point dir) {
	player.move(player.position() + (dir - player.position()).norm(INCREMENT), player.orientation(), Point());
	return;
}

void Action::strafe(Player player, const Point dir, const Angle face) {
	player.move(player.position() + (dir - player.position()).norm(INCREMENT), face, Point());
	return;
}

void Action::strafe(Player player, const Point dir, const Point face) {
	player.move(player.position() + (dir - player.position()).norm(INCREMENT), (face - player.position()).orientation(), Point());
	return;
}
