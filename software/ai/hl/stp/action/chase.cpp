#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::chase(Player::Ptr player, Point target) {
	player->move(target, (target - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::CATCH_PIVOT);
	player->prio(AI::Flags::MovePrio::HIGH);
}

