#include "ai/flags.h"
#include "ai/hl/stp/action/move_spin.h"
#include "geom/angle.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::move_spin(Player::Ptr player, const Point dest) {
	player->move(dest, angle_mod(player->orientation() + 0.1), Point());
	player->type(AI::Flags::MoveType::NORMAL);
}
