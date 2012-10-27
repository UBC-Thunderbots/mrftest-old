#include "ai/hl/stp/tactic/intercept.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class Intercept : public Tactic {
		public:
			Intercept(World world, const Point target) : Tactic(world, true), target(target) {
			}

		private:
			bool done() const;
			Player select(const std::set<Player> &players) const;
			void execute();
			const Point target;
			Glib::ustring description() const {
				return "intercept";
			}
			// void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
	};

	bool Intercept::done() const {
		return player.has_ball();
	}

	Player Intercept::select(const std::set<Player> &players) const {
		return select_baller(world, players, player);
	}

	void Intercept::execute() {
		// if it has the ball, stay there
		if (player.has_ball()) {
			Action::dribble(world, player);
			return;
		}

		// orient towards the enemy goal?
		Action::intercept(player, target);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::intercept(World world, const Point target) {
	Tactic::Ptr p(new Intercept(world, target));
	return p;
}

