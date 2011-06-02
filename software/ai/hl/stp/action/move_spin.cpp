#include "ai/flags.h"
#include "ai/hl/stp/action/move_spin.h"
#include "geom/angle.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::move_spin(Player::Ptr player, const Point dest) {
	// spin faster when you are close to the destination point
	if ((player->position() - dest).len() < AI::HL::Util::POS_CLOSE) {
		player->move(dest, angle_mod(player->orientation() + 0.4), Point());
	} else {
		player->move(dest, angle_mod(player->orientation() + 0.2), Point());
	}
	player->type(AI::Flags::MoveType::NORMAL);
}

