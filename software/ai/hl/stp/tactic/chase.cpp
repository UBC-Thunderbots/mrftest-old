#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/tactic/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;

namespace {
	class Chase : public Tactic {
		public:
			Chase(const World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "chase";
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
	};

	bool Chase::done() const {
		return player->has_ball();
	}

	Player::Ptr Chase::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players);
	}

	void Chase::execute() {
		// if it has the ball, stay there
		if (player->has_ball()) {
			Action::dribble(world, player);
			return;
		}

		// TODO: flags
		//Action::chase(world, player, 0);
		
		// orient towards the enemy goal?
		Action::chase(world, player, world.field().enemy_goal());
	}

	void Chase::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
		ctx->set_source_rgb(1.0, 1.0, 1.0);
		ctx->move_to(player->position().x, player->position().y);
		ctx->line_to(world.ball().position().x, world.ball().position().y);
		ctx->set_line_width(0.01);
		ctx->stroke();
	}
}

Tactic::Ptr AI::HL::STP::Tactic::chase(const World &world) {
	const Tactic::Ptr p(new Chase(world));
	return p;
}

