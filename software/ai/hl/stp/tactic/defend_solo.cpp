#include "defend_solo.h"

#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/util.h"
#include <cassert>
#include "../action/goalie.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class LoneGoalie final : public Tactic {
		public:
			explicit LoneGoalie(World world, bool active) : Tactic(world) {
			}

		private:
			bool done() const override;
			void execute(caller_t& ca) override;
			Player select(const std::set<Player> &) const override {
				assert(false);
			}
			Glib::ustring description() const override {
				return u8"goalie (all alone)";
			}
	};

	bool LoneGoalie::done() const {
		return false;
	}

	void LoneGoalie::execute(caller_t& ca) {
		while(true) {
			AI::HL::STP::Action::lone_goalie(ca, world, player());
			yield(ca);
		}

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

