#include "ai/flags.h"
#include "ai/hl/stp/action/move_spin.h"
#include "geom/angle.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam spin_delta("change in orientation every time tick for move spin", "STP/Action/spin", 1.0, 0.5, 2.0);

}

void AI::HL::STP::Action::move_spin(Player::Ptr player, const Point dest) {
	// spin faster when you are close to the destination point
	if ((player->position() - dest).len() < AI::HL::Util::POS_CLOSE) {
		player->move(dest, angle_mod(player->orientation() + 2 * spin_delta), Point());
	} else {
		player->move(dest, angle_mod(player->orientation() + spin_delta), Point());
	}
	player->type(AI::Flags::MoveType::RAM_BALL);
}

