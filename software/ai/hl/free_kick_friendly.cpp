#include "ai/hl/free_kick_friendly.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/algorithm.h"
#include "util/dprint.h"

#include <vector>

using AI::HL::DirectFreeKickFriendly;
using AI::HL::IndirectFreeKickFriendly;
using namespace AI::HL::W;

DirectFreeKickFriendly::DirectFreeKickFriendly(World &w) : world(w) {
}

void DirectFreeKickFriendly::tick() {
#warning Under Construction
	// const FriendlyTeam &friendly = world.friendly_team();
	std::vector<Player::Ptr> friendly = AI::HL::Util::get_players(world.friendly_team());

	if (!kicker.is()) {
		LOG_ERROR("no goalie");
		return;
	}

	std::vector<Player::Ptr> friends;
	for (size_t i = 0; i < friendly.size(); ++i) {
		if (friendly[i] != kicker) {
			friends.push_back(friendly[i]);
		}
	}

	// should do some positioning of players?

	std::pair<Point, double> bestshot = AI::HL::Util::calc_best_shot(world, kicker);

	AI::HL::Tactics::shoot(world, kicker, AI::Flags::FLAG_CLIP_PLAY_AREA, bestshot.first);
}

IndirectFreeKickFriendly::IndirectFreeKickFriendly(World &w) : world(w) {
}

void IndirectFreeKickFriendly::tick() {
#warning Under Construction
	// const FriendlyTeam &friendly = world.friendly_team();
	std::vector<Player::Ptr> friendly = AI::HL::Util::get_players(world.friendly_team());

	if (!kicker.is()) {
		LOG_ERROR("no goalie");
		return;
	}

	std::vector<Player::Ptr> friends;
	for (size_t i = 0; i < friendly.size(); ++i) {
		if (friendly[i] != kicker) {
			friends.push_back(friendly[i]);
		}
	}

	// should do some positioning of players?

	LOG_INFO("forced kicking");
	AI::HL::Tactics::shoot(world, kicker, AI::Flags::FLAG_CLIP_PLAY_AREA);
}

