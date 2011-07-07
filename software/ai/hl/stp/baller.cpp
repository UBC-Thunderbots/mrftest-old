#include "ai/hl/stp/baller.h"
#include "ai/util.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/param.h"

using namespace AI::HL::STP;

namespace {
	Player::CPtr baller;
}

Player::CPtr AI::HL::STP::select_friendly_baller(const World &world) {
	Player::CPtr best;
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (friendly.get(i)->has_ball()) {
			best = friendly.get(i);
			if (best == baller) {
				return baller;
			}
		}
	}
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (Evaluation::possess_ball(world, friendly.get(i))) {
			if (best == baller) {
				return baller;
			}
		}
	}
	baller = best;
	if (baller.is()) {
		return baller;
	}

	double min_dist = 1e99;
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		Player::CPtr player = friendly.get(i);
		Point dest = Evaluation::calc_fastest_grab_ball_dest(world, player);
		if (!best.is() || min_dist > (dest - player->position()).len()) {
			min_dist = (dest - player->position()).len();
			best = player;
		}
	}

	baller = best;
	return best;
}

