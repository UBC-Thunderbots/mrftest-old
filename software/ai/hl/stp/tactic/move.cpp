#include <algorithm>

#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Action = AI::HL::STP::Action;

namespace {
	class Move final : public Tactic {
		public:
			explicit Move(World world, const Coordinate dest) : Tactic(world), dest(dest) {
			}

		private:
			const Coordinate dest;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override {
				return u8"move";
			}
	};

	Player Move::select(const std::set<Player> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest.position()));
	}

	void Move::execute() {
		Action::move(world, player, dest.position());
	}

	class GoalieMove final : public Tactic {
		public:
			explicit GoalieMove(World world, const Coordinate dest) : Tactic(world), dest(dest) {
			}

		private:
			const Coordinate dest;
			Player select(const std::set<Player> &) const override {
				assert(false);
			}
			void execute() override;
			Glib::ustring description() const override {
				return u8"goalie move";
			}
	};

	void GoalieMove::execute() {
		Action::goalie_move_direct(world, player, dest.position());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move(World world, const Coordinate dest) {
	Tactic::Ptr p(new Move(world, dest));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::goalie_move(World world, const Coordinate dest) {
	Tactic::Ptr p(new GoalieMove(world, dest));
	return p;
}

