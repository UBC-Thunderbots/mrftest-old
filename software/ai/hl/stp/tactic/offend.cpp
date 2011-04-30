#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;

namespace {
	class Primary : public Tactic {
		public:
			Primary(const World &world) : Tactic(world) {
			}

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "offender";
			}
	};

	class Secondary : public Tactic {
		public:
			Secondary(const World &world) : Tactic(world) {
			}

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "offender (secondary)";
			}
	};

	Player::Ptr Primary::select(const std::set<Player::Ptr> &players) const {
		auto dest = AI::HL::STP::Evaluation::offense_positions(world);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest[0]));
	}

	void Primary::execute() {
		auto dest = AI::HL::STP::Evaluation::offense_positions(world);
		Action::move(world, player, dest[0]);
		player->prio(AI::Flags::MovePrio::LOW);
	}

	Player::Ptr Secondary::select(const std::set<Player::Ptr> &players) const {
		auto dest = AI::HL::STP::Evaluation::offense_positions(world);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest[1]));
	}

	void Secondary::execute() {
		auto dest = AI::HL::STP::Evaluation::offense_positions(world);
		Action::move(world, player, dest[1]);
		player->prio(AI::Flags::MovePrio::LOW);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::offend(const World &world) {
	const Tactic::Ptr p(new Primary(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::offend_secondary(const World &world) {
	const Tactic::Ptr p(new Secondary(world));
	return p;
}

