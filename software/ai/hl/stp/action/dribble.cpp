#include "ai/hl/stp/action/dribble.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::dribble(const World &world, Player::Ptr player, const Point dest) {
	player->move(dest, (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::DRIBBLE);
	player->prio(AI::Flags::MovePrio::HIGH);
}

void AI::HL::STP::Action::dribble(const World &world, Player::Ptr player) {
	dribble(world, player, player->position());
}

