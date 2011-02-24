#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

Player::Ptr AI::HL::STP::Tactic::select_baller(const World& world, const std::set<Player::Ptr> &players) {
	for (auto it = players.begin(); it != players.end(); ++it) {
		if ((*it)->has_ball()) {
			return *it;
		}
	}
	return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
}

