#include "ai/hl/stp/tactic/offense.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Offense : public Tactic {
		public:
			Offense(World &world) : Tactic(world) {
			}

		private:
#warning mutable... maybe i should fix api
			mutable Point dest;

			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	Player::Ptr Offense::select(const std::set<Player::Ptr> &players) const {
#warning a tactic can make decision based on available players
		dest = AI::HL::STP::Evaluation::evaluate_offense(world, players);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
	}

	void Offense::execute() {
		player->move(dest, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::offense(World &world) {
	const Tactic::Ptr p(new Offense(world));
	return p;
}

