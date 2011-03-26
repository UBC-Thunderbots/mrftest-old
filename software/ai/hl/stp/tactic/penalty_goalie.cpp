#include "ai/hl/stp/tactic/penalty_goalie.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/util.h"

#include <cassert>

using namespace AI::HL::W;
using namespace AI::HL::STP::Tactic;

namespace {
	class PenaltyGoalie : public Tactic {
		public:
			PenaltyGoalie(const World &world);
		private:
			bool goto_target1;
			bool done() const;
			void execute();
			Player::Ptr select(const std::set<Player::Ptr> &) const {
				assert(0);
			}
	};

	PenaltyGoalie::PenaltyGoalie(const World& world) : Tactic(world, true) {
	}

	bool PenaltyGoalie::done() const {
		// it's never done!
		return false;
	}

	void PenaltyGoalie::execute() {
		AI::HL::STP::Action::penalty_goalie(world, player);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_goalie(const World &world) {
	const Tactic::Ptr p(new PenaltyGoalie(world));
	return p;
}

