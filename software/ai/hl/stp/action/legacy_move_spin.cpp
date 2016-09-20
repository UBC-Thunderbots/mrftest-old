#include "ai/hl/stp/action/legacy_action.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/legacy_move_spin.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/param.h"

using namespace AI::HL::STP;

namespace {
	RadianParam spin_delta(u8"change in orientation every time tick for move spin (radians)", u8"AI/HL/STP/Action/spin", 600.0, 0.0, 1200.0);

	const int CLOCKWISE = -1;
	const int COUNTER_CLOCKWISE = 1;
}

void AI::HL::STP::Action::move_spin(Player player, const Point dest) {
	// spin in different directions depending on which quadrant of the field the robot is at
	int direction_to_spin;

	if (player.position().x < 0) {
		if (player.position().y < 0) {
			direction_to_spin= CLOCKWISE;
		} else {
			direction_to_spin= COUNTER_CLOCKWISE;
		}
	} else {
		if (player.position().y < 0) {
			direction_to_spin= COUNTER_CLOCKWISE;
		} else {
			direction_to_spin= CLOCKWISE;
		}
	}

	// spin faster when you are close to the destination point
	player.avoid_distance(AI::Flags::AvoidDistance::SHORT);
	if ((player.position() - dest).len() < AI::HL::Util::POS_CLOSE + Robot::MAX_RADIUS) {
		player.mp_spin(dest, spin_delta*direction_to_spin);
	} else {
		player.mp_spin(dest, spin_delta*direction_to_spin);
	}

	player.prio(AI::Flags::MovePrio::HIGH);
}

