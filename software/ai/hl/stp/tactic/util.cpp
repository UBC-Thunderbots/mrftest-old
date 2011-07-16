#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/team.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Player::Ptr AI::HL::STP::Tactic::select_baller(const World &world, const std::set<Player::Ptr> &players, Player::Ptr previous) {
	// if someone has ball, use it
	for (auto it = players.begin(); it != players.end(); ++it) {
		Player::Ptr p = *it;
		if (p->has_ball()) {
			return p;
		}
	}
	// possess ball
	Player::Ptr best;
	for (auto it = players.begin(); it != players.end(); ++it) {
		Player::Ptr p = *it;
		Player::CPtr p_c = p;
		if (!Evaluation::possess_ball(world, p_c)) {
			continue;
		}
		if (p == previous) {
			return p;
		}
		best = p;
	}

	if (best.is()) {
		return best;
	}

	double min_dist = 1e99;
	for (auto it = players.begin(); it != players.end(); ++it) {
		Player::Ptr player = *it;
		Point dest = Evaluation::calc_fastest_grab_ball_dest(world, player);
		if (!best.is() || min_dist > (dest - player->position()).len()) {
			min_dist = (dest - player->position()).len();
			best = player;
		}
	}

	return best;
}

