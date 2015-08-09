#include "ai/hl/stp/action/strafe.h"
#include "ai/flags.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam INCREMENT(u8"strafing increment", u8"AI/HL/STP/Action/Strafe", 0.1, 0.01, 1);
}

void Action::strafe(Player player, const Point dir) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	player.mp_move(player.position() + (dir - player.position()).norm(INCREMENT), player.orientation());
	return;
}

void Action::strafe(Player player, const Point dir, const Angle face) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	player.mp_move(player.position() + (dir - player.position()).norm(INCREMENT), face);
	return;
}

void Action::strafe(Player player, const Point dir, const Point face) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	player.mp_move(player.position() + (dir - player.position()).norm(INCREMENT), (face - player.position()).orientation());
	return;
}
