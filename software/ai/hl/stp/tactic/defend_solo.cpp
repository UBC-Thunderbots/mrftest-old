#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/action/goalie.h"
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
			void execute();
			Player::Ptr select(const std::set<Player::Ptr> &) const {
				assert(0);
			}
			std::string description() const {
				return "lone-goalie";
			}
	};

	void SoloGoalie::execute() {
		AI::HL::STP::Action::lone_goalie(world, player);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::defend_solo_goalie(const World &world) {
	const Tactic::Ptr p(new SoloGoalie(world));
	return p;
}

