#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include <cmath>
#include <algorithm>

void AI::HL::STP::Action::pivot(const World &world, Player::Ptr player, const Point target) {
	if (!player->has_ball()) {
		chase(world, player);
		return;
	}

	const double ori = (target - player->position()).orientation();

	player->move(player->position(), ori, Point());
	player->type(AI::Flags::MoveType::PIVOT);
	player->prio(AI::Flags::MovePrio::HIGH);
}

