#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/util.h"
#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class LoneGoalie : public Tactic {
		public:
			LoneGoalie(World world, bool active) : Tactic(world, active) {
			}

		private:
			bool done() const;
			void execute();
			Player select(const std::set<Player> &) const {
				assert(false);
			}
			Glib::ustring description() const {
				return u8"goalie (all alone)";
			}
	};

	bool LoneGoalie::done() const {
		return false;
	}

	void LoneGoalie::execute() {
		AI::HL::STP::Action::lone_goalie(world, player);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::lone_goalie(World world) {
	Tactic::Ptr p(new LoneGoalie(world, false));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::lone_goalie_active(World world) {
	Tactic::Ptr p(new LoneGoalie(world, true));
	return p;
}

