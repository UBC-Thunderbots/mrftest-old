#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include <cmath>
#include <algorithm>

void AI::HL::STP::Action::chase_pivot(const World &world, Player::Ptr player, const Point target) {
	if (Evaluation::ball_in_pivot_thresh(world, player)) {
		pivot(world, player, target);
	} else {
		chase(world, player, target);
	}
}

void AI::HL::STP::Action::pivot(const World &world, Player::Ptr player, const Point target) {

	const double ori = (target - player->position()).orientation();

	player->move(player->position(), ori, target);
	player->type(AI::Flags::MoveType::PIVOT);
	player->prio(AI::Flags::MovePrio::HIGH);
}

