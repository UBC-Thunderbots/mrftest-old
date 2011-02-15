#include "ai/hl/stp/tactic/lone_goalie.h"
#include "ai/hl/old/tactics.h"
#include "ai/hl/util.h"
#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class LoneGoalie : public Tactic {
		public:
			LoneGoalie(const World &world) : Tactic(world) {
			}

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	Player::Ptr LoneGoalie::select(const std::set<Player::Ptr> &players) const {
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

	void LoneGoalie::execute() {
		// TODO: use proper skill
		AI::HL::Tactics::lone_goalie(world, player);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::lone_goalie(const World &world) {
	const Tactic::Ptr p(new LoneGoalie(world));
	return p;
}

