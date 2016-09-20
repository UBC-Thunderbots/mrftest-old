#include "ai/hl/stp/action/legacy_action.h"
#include "ai/hl/stp/action/legacy_stop.h"
#include "ai/flags.h"
#include "util/param.h"
#include "ai/hl/stp/action/legacy_action.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::stop(World, Player player) {
	player.mp_move(player.position(), player.orientation());
	player.prio(AI::Flags::MovePrio::LOW);
}
