#include "ai/hl/stp/tactic/legacy_intercept.h"
#include "ai/hl/stp/action/legacy_intercept.h"
#include "ai/hl/stp/action/legacy_dribble.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/util.h"

#include "ai/hl/stp/tactic/legacy_tactic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class Intercept final : public LegacyTactic {
		public:
			explicit Intercept(World world, const Point target) : LegacyTactic(world, true), target(target) {
			}
			explicit Intercept(World world) : LegacyTactic(world, true) {
				target = world.field().enemy_goal();
			}
		private:
			bool done() const override;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Point target;
			Glib::ustring description() const override {
				return u8"intercept";
			}
			// void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
	};

	bool Intercept::done() const {
		return player && player.has_ball();
	}

	Player Intercept::select(const std::set<Player> &players) const {
		return select_baller(world, players, player);
	}

	void Intercept::execute() {
		// if it has the ball, stay there
		if (player.has_ball()) {
			Action::dribble(player);
			return;
		}

		// orient towards the enemy goal?
		Action::intercept(player, world.ball().position());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::intercept(World world, const Point target) {
	Tactic::Ptr p(new Intercept(world, target));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::intercept(World world) {
	Tactic::Ptr p(new Intercept(world));
	return p;
}
