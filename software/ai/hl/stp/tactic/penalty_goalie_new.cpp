#include "ai/hl/stp/tactic/penalty_goalie_new.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/param.h"
#include "geom/util.h"

#include <cassert>

using namespace AI::HL::W;
using namespace AI::HL::STP::Tactic;

namespace {
	Point old_des;

	class PenaltyGoalieNew : public Tactic {
		public:
			PenaltyGoalieNew(World world);
		private:
			bool done() const;
			void execute();
			double power;
			Robot shooter;
			Point predicted;
			Player select(const std::set<Player> &) const {
				assert(false);
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> context) const;
			Glib::ustring description() const {
				return "penalty-goalie-new";
			}
	};

	PenaltyGoalieNew::PenaltyGoalieNew(World world) : Tactic(world, true), predicted(Point(0, 0)) {
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

		


		AI::HL::STP::Action::move(player, (world.ball().position() - player.position()).orientation(), clip_point(line_intersect(world.field().friendly_goal_boundary().first, world.field().friendly_goal_boundary().second, world.ball().position(), predicted), world.field().friendly_goal_boundary().first + Point(0, goalie_range), world.field().friendly_goal_boundary().second - Point(0.00001, goalie_range)) + Point(Robot::MAX_RADIUS, 0));

		if (player.has_ball())
			player.autochip(power);
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

