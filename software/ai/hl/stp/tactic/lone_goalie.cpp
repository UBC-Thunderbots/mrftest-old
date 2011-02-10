#include "ai/hl/stp/tactic/lone_goalie.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class LoneGoalie : public Tactic {
		public:
			LoneGoalie(World &world) : Tactic(world) {
			}
		private:
			double score(Player::Ptr player) const;
			void execute();
	};

	double LoneGoalie::score(Player::Ptr player) const {
		if (world.friendly_team().get(0) == player) {
			return 1;
		}
		return 0;
	}

	void LoneGoalie::execute() {
		// TODO: use proper skill
		AI::HL::Tactics::lone_goalie(world, player);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::lone_goalie(World &world) {
	const Tactic::Ptr p(new LoneGoalie(world));
	return p;
}

