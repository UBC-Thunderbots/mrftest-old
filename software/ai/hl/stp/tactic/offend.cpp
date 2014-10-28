#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;

namespace {
	class Primary final : public Tactic {
		public:
			explicit Primary(World world) : Tactic(world) {
			}

		private:
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override {
				return u8"offender (primary)";
			}
	};

	class Secondary final : public Tactic {
		public:
			explicit Secondary(World world) : Tactic(world) {
			}

		private:
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override {
				return u8"offender (secondary)";
			}
	};

	Player Primary::select(const std::set<Player> &players) const {
		auto dest = AI::HL::STP::Evaluation::offense_positions();
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest[0]));
	}

	void Primary::execute() {
		auto dest = AI::HL::STP::Evaluation::offense_positions();
		Action::move(world, player, dest[0]);
		player.prio(AI::Flags::MovePrio::LOW);
		player.dribble(AI::BE::Player::DribbleMode::STOP);
	}

	Player Secondary::select(const std::set<Player> &players) const {
		auto dest = AI::HL::STP::Evaluation::offense_positions();
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest[1]));
	}

	void Secondary::execute() {
		auto dest = AI::HL::STP::Evaluation::offense_positions();
		Action::move(world, player, dest[1]);
		player.prio(AI::Flags::MovePrio::LOW);
		player.dribble(AI::BE::Player::DribbleMode::STOP);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::offend(World world) {
	Tactic::Ptr p(new Primary(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::offend_secondary(World world) {
	Tactic::Ptr p(new Secondary(world));
	return p;
}

