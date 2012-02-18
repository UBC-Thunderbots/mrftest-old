#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class Chase : public Tactic {
		public:
			Chase(const World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			Glib::ustring description() const {
				return "chase";
			}
			// void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
	};

	bool Chase::done() const {
		return player->has_ball();
	}

	Player::Ptr Chase::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players, player);
	}

	void Chase::execute() {
		// if it has the ball, stay there
		if (player->has_ball()) {
			Action::dribble(world, player);
			return;
		}

		// orient towards the enemy goal?
		Action::chase(player, world.field().enemy_goal());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::chase(const World &world) {
	Tactic::Ptr p(new Chase(world));
	return p;
}

