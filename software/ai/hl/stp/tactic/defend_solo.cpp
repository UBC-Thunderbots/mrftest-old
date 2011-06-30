#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/util.h"
#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class LoneGoalie : public Tactic {
		public:
			LoneGoalie(const World &world, bool active) : Tactic(world, active) {
			}

		private:
			bool done() const;
			void execute();
			Player::Ptr select(const std::set<Player::Ptr> &) const {
				assert(0);
			}
			std::string description() const {
				return "goalie (all alone)";
			}
	};

	bool LoneGoalie::done() const {
		return false;
	}

	void LoneGoalie::execute() {
		AI::HL::STP::Action::lone_goalie(world, player);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::lone_goalie(const World &world) {
	const Tactic::Ptr p(new LoneGoalie(world, false));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::lone_goalie_active(const World &world) {
	const Tactic::Ptr p(new LoneGoalie(world, true));
	return p;
}

