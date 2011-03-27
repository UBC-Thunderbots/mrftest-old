#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Offend : public Tactic {
		public:
			Offend(const World &world) : Tactic(world) {
			}

		private:
#warning mutable... maybe i should fix api
			mutable Point dest;

			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "offender";
			}
	};

	Player::Ptr Offend::select(const std::set<Player::Ptr> &players) const {
#warning a tactic can make decision based on available players
		dest = AI::HL::STP::Evaluation::evaluate_offense(world, players);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
	}

	void Offend::execute() {
		player->move(dest, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::offend(const World &world) {
	const Tactic::Ptr p(new Offend(world));
	return p;
}

