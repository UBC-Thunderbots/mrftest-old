#include "ai/flags.h"
#include "ai/hl/stp/action/move_spin.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/param.h"

using namespace AI::HL::STP;

namespace {
	const int CLOCKWISE = -1;
	const int COUNTER_CLOCKWISE = 1;
}

void AI::HL::STP::Action::move_spin(caller_t& ca, Player player, Point dest, Angle speed) {
	// spin in different directions depending on which quadrant of the field the robot is at
	int direction_to_spin;

	if (player.position().x < 0) {
		if (player.position().y < 0) {
			direction_to_spin = CLOCKWISE;
		} else {
			direction_to_spin = COUNTER_CLOCKWISE;
		}
	} else {
		if (player.position().y < 0) {
			direction_to_spin = COUNTER_CLOCKWISE;
		} else {
			direction_to_spin = CLOCKWISE;
		}
	}

	player.prio(AI::Flags::MovePrio::HIGH);

	const Primitive& prim = Primitives::Spin(player, dest, direction_to_spin * speed.abs());
	Action::wait(ca, prim);
}

