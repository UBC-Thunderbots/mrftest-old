#include "ai/flags.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::dribble(World world, Player player, const Point dest) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	player.mp_dribble(dest, (world.ball().position() - player.position()).orientation());
	player.type(AI::Flags::MoveType::DRIBBLE);
	player.prio(AI::Flags::MovePrio::HIGH);
}

void AI::HL::STP::Action::dribble(Player player) {
	// Avoid defense areas
	player.flags(AI::Flags::FLAG_AVOID_FRIENDLY_DEFENSE ||
		AI::Flags::FLAG_AVOID_ENEMY_DEFENSE);

	player.mp_dribble(player.position(), player.orientation());
}

