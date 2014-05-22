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
			Player select(const std::set<Player> &) const {
				assert(false);
			}
			Glib::ustring description() const {
				return "penalty-goalie-new";
			}
	};

	PenaltyGoalieNew::PenaltyGoalieNew(World world) : Tactic(world, true) {
	}

	bool PenaltyGoalieNew::done() const {
		// it's never done!
		return false;
	}

	void PenaltyGoalieNew::execute() {
		std::vector<Point> goalie_vec;
		   Robot shooter;
		   for(auto i : world.enemy_team()) {
			   goalie_vec.push_back(i.position());
			   shooter = i;
		   }

		   AI::HL::STP::Action::move(player, Angle::quarter(), clip_point(line_intersect(world.field().friendly_goal_boundary().first, world.field().friendly_goal_boundary().second, Point(shooter.orientation().cos(), shooter.orientation().sin()), shooter.position()), world.field().friendly_goal_boundary().first + Point(0, 0.13), world.field().friendly_goal_boundary().second - Point(0.00001, 0.13)) + Point(0.03, 0));


		if (player.has_ball())
				player.autochip(power);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_goalie_new(World world) {
	Tactic::Ptr p(new PenaltyGoalieNew(world));
	return p;
}

