#include "ai/hl/stp/tactic/repel.h"
#include "ai/hl/stp/action/actions.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Repel : public Tactic {
		public:
			Repel(const World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	bool Repel::done() const {
#warning TODO
		// the ball is in some safe state:
		// far away from goalie
		// not rolling towards the defense area at all
		if (world.ball().position().x > 0) return true;
		return false;
	}

	Player::Ptr Repel::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
	}

	void Repel::execute() {
		AI::HL::STP::Action::repel(world, player, 0);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::repel(const World &world) {
	const Tactic::Ptr p(new Repel(world));
	return p;
}

