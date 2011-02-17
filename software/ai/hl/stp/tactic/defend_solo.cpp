#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/old/tactics.h"
#include "ai/hl/util.h"
#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	/**
	 * TODO: maybe make it active?
	 */
	class SoloGoalie : public Tactic {
		public:
			SoloGoalie(const World &world) : Tactic(world) {
			}

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	Player::Ptr SoloGoalie::select(const std::set<Player::Ptr> &players) const {
		Player::CPtr cgoalie = world.friendly_team().get(0);
		Player::Ptr goalie;
		for (auto it = players.begin(); it != players.end(); ++it) {
			Player::CPtr tmp = *it;
			if (cgoalie == tmp) {
				goalie = *it;
			}
		}
		assert(goalie.is());
		return goalie;
	}

	void SoloGoalie::execute() {
#warning use goalie SSM
		AI::HL::Tactics::lone_goalie(world, player);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::defend_solo_goalie(const World &world) {
	const Tactic::Ptr p(new SoloGoalie(world));
	return p;
}

