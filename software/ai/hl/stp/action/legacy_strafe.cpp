#include "ai/hl/stp/action/legacy_action.h"
#include "ai/hl/stp/action/legacy_strafe.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam INCREMENT(u8"strafing increment", u8"AI/HL/STP/Action/Strafe", 0.1, 0.01, 1);
}

void Action::strafe(Player player, const Point dir) {
	player.mp_move(player.position() + (dir - player.position()).norm(INCREMENT), player.orientation());
	return;
}

void Action::strafe(Player player, const Point dir, const Angle face) {
	player.mp_move(player.position() + (dir - player.position()).norm(INCREMENT), face);
	return;
}

void Action::strafe(Player player, const Point dir, const Point face) {
	player.mp_move(player.position() + (dir - player.position()).norm(INCREMENT), (face - player.position()).orientation());
	return;
}

void Action::strafe_dribble(Player player, const Point dir, const Point face) {
	player.mp_dribble(player.position() + (dir - player.position()).norm(INCREMENT), (face - player.position()).orientation());
	return;
}
