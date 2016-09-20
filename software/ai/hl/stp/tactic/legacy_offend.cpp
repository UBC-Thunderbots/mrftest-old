#include "ai/hl/stp/tactic/legacy_offend.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/action/legacy_move.h"
#include "ai/hl/util.h"

#include "ai/hl/stp/tactic/legacy_tactic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;

namespace {
	class Primary final : public LegacyTactic {
		public:
			explicit Primary(World world) : LegacyTactic(world) {
			}

		private:
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override {
				return u8"offender (primary)";
			}
	};

	class Secondary final : public LegacyTactic {
		public:
			explicit Secondary(World world) : LegacyTactic(world) {
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
	}

	Player Secondary::select(const std::set<Player> &players) const {
		auto dest = AI::HL::STP::Evaluation::offense_positions();
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest[1]));
	}

	void Secondary::execute() {
		auto dest = AI::HL::STP::Evaluation::offense_positions();
		Action::move(world, player, dest[1]);
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

