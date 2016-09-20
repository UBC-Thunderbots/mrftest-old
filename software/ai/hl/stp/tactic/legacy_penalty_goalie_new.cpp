#include "ai/hl/stp/tactic/legacy_penalty_goalie_new.h"
#include "ai/hl/stp/action/legacy_goalie.h"
#include "ai/hl/stp/action/legacy_move.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/param.h"
#include "geom/util.h"
#include "ai/hl/stp/action/legacy_chip.h"

#include "ai/hl/stp/tactic/legacy_tactic.h"

#include <cassert>

using namespace AI::HL::W;
using namespace AI::HL::STP::Tactic;

namespace {
	Point old_des;

	class PenaltyGoalieNew final : public LegacyTactic {
		public:
			explicit PenaltyGoalieNew(World world);
		private:
			bool done() const override;
			void execute() override;
			double power;
			Robot shooter;
			Point predicted;
			Player select(const std::set<Player> &) const override {
				assert(false);
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> context) const override;
			Glib::ustring description() const override {
				return "penalty-goalie-new";
			}
	};

	PenaltyGoalieNew::PenaltyGoalieNew(World world) : LegacyTactic(world, true), predicted(Point(0, 0)) {
	}

	bool PenaltyGoalieNew::done() const {
		// it's never done!
		return false;
	}

	void PenaltyGoalieNew::execute() {
		double goalie_range = 0.5 * (world.field().friendly_goal_boundary().second - world.field().friendly_goal_boundary().first).y - Robot::MAX_RADIUS - 0.10;

		double best_distance = 99; //to determine which robot is shooting

		if(world.enemy_team().size() < 1)
			return;

		for(auto i : world.enemy_team()) {
			if((i.position() - world.ball().position()).len() < best_distance) {
				shooter = i;
				best_distance = (shooter.position() - world.ball().position()).len();
				predicted = shooter.position() + shooter.velocity() * 0.05;
			}
		}

		


		AI::HL::STP::Action::move(world, player, clip_point(line_intersect(world.field().friendly_goal_boundary().first, world.field().friendly_goal_boundary().second, world.ball().position(), predicted), world.field().friendly_goal_boundary().first + Point(0, goalie_range), world.field().friendly_goal_boundary().second - Point(0.00001, goalie_range)) + Point(Robot::MAX_RADIUS, 0), (world.ball().position() - player.position()).orientation());

		if (player.has_ball()) {
			AI::HL::STP::Action::goalie_chip_target(world, player, world.field().enemy_goal());
		}
	}

	void PenaltyGoalieNew::draw_overlay(Cairo::RefPtr<Cairo::Context> context) const {
		context->set_line_width(0.02);
		context->set_source_rgb(1.0, 0, 1.0);
		context->arc(predicted.x, predicted.y, Robot::MAX_RADIUS * 1.2, 0.0, 2 * M_PI);
		context->stroke();
	}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_goalie_new(World world) {
	Tactic::Ptr p(new PenaltyGoalieNew(world));
	return p;
}

