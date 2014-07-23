#include "ai/hl/stp/tactic/penalty_goalie_random.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/param.h"

#include <cassert>

using namespace AI::HL::W;
using namespace AI::HL::STP::Tactic;

namespace {
	Point old_des;

	class PenaltyGoalieRandom : public Tactic {
		public:
			PenaltyGoalieRandom(World world);

		private:
			bool goto_target1;
			bool done() const;
			void execute();
//better implementation of chip power required. perhaps make a namespace variable for power?
			double power;
			Player select(const std::set<Player> &) const {
				assert(false);
			}

			Robot shooter;
			Glib::ustring description() const {
				return "penalty-goalie";
			}
	};
}

	PenaltyGoalieRandom::PenaltyGoalieRandom(World world) : Tactic(world, true), goto_target1(false), power(0.6) {
		old_des = Point(world.field().friendly_goal().x + Robot::MAX_RADIUS, -0.8 * Robot::MAX_RADIUS);
	}

	bool PenaltyGoalieRandom::done() const {
		// it's never done!
		return false;
	}

	void PenaltyGoalieRandom::execute() {
//			Point top = Point(world.field().friendly_goal_boundary().first - 1 * Robot::MAX_RADIUS);
//			Point bottom = Point(world.field().friendly_goal_boundary().second + 1 * Robot::MAX_RADIUS);
//			Point next_point;
//
//			double goalie_range = 0.5 * (world.field().friendly_goal_boundary().second - world.field().friendly_goal_boundary().first).y - Robot::MAX_RADIUS - 0.10;
//
//			double best_distance = 99; //to determine which robot is shooting
//
//			if(world.enemy_team().size() < 1)
//				return;
//
//			for(auto i : world.enemy_team()) {
//				if((i.position() - world.ball().position()).len() < best_distance) {
//					shooter = i;
//					best_distance = (shooter.position() - world.ball().position()).len();
//					predicted = shooter.position() + shooter.velocity() * 0.05;
//				}
//			}
//
//			shooter.orientation()
//			// uniform random generator - use timestamp as seed
//			unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
//			std::default_random_engine generator (seed);
//
//			next_point.y = generator % (top.y - bottom.y) + bottom.y;
//			next_point.x = player.position().x;
//


//			// just orient towards the "front"
//			player.move(next_point, Angle::zero(), Point());
			player.type(AI::Flags::MoveType::RAM_BALL);
			player.prio(AI::Flags::MovePrio::HIGH);
		if (player.has_ball())
				player.autochip(power);
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_goalie_random(World world) {
	Tactic::Ptr p(new PenaltyGoalieRandom(world));
	return p;
}

