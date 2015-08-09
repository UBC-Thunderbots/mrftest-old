#include "ai/hl/stp/action/stop.h"
#include "ai/flags.h"
#include "util/param.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::stop(World, Player player) {
	player.mp_move(player.position(), player.orientation());
	player.type(AI::Flags::MoveType::NORMAL);
	player.prio(AI::Flags::MovePrio::LOW);
}

