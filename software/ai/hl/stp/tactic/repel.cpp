#include "ai/hl/stp/tactic/repel.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Repel : public Tactic {
		public:
			Repel(World &world) : Tactic(world) {
			}
		private:
			Player::Ptr select(const std::set<Player::Ptr>& players) const;
			void execute();
	};

	Player::Ptr Repel::select(const std::set<Player::Ptr>& players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
	}

	void Repel::execute() {
		// TODO: use proper skill
		// use bump to goal or drive to goal with move type RAM_BALL?
		AI::HL::Tactics::repel(world, player, 0);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::repel(World &world) {
	const Tactic::Ptr p(new Repel(world));
	return p;
}

