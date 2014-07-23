#include "ai/hl/stp/tactic/penalty_goalie_random.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/param.h"

#include <cassert>

using namespace AI::HL::W;
using namespace AI::HL::STP::Tactic;

namespace {
	DoubleParam PARAM_penalty_stdev(u8"standard deviation for goalie movement (sec divided by 30)", u8"AI/penalty", 0.1, 0.0, 10.0);
	DoubleParam PARAM_penalty_travel(u8"travel width for goalie movement (robot width)", u8"AI/penalty", 2.0, 0.0, 10.0);

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

			int counter;
			bool is_left;
			Point left_target;
			Point right_target;

			Robot shooter;
			Glib::ustring description() const {
				return u8"penalty-goalie-random";
			}
	};
}

	PenaltyGoalieRandom::PenaltyGoalieRandom(World world) : Tactic(world, true), goto_target1(false), power(0.6) {
		old_des = Point(world.field().friendly_goal().x + Robot::MAX_RADIUS, -0.8 * Robot::MAX_RADIUS);
		counter = 0;
		is_left=true;
		left_target = Point(world.field().friendly_goal().x + Robot::MAX_RADIUS, PARAM_penalty_travel*Robot::MAX_RADIUS);
		right_target = Point(world.field().friendly_goal().x + Robot::MAX_RADIUS, -1*PARAM_penalty_travel*Robot::MAX_RADIUS);
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

    		// uniform random generator - use timestamp as seed
			counter--;

			if(is_left){
				player.move(left_target,Angle::zero(), Point());
			}
			else {
				player.move(right_target,Angle::zero(), Point());
			}

			if( counter == 0 ){

				unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
				std::default_random_engine generator (seed);
				std::normal_distribution<double> normal(0, PARAM_penalty_stdev);
				double time = normal(generator);

				counter = (int)(time*30);

				if( is_left ){
					is_left = false;
				}else{
					is_left = true;
				}

			}
			player.type(AI::Flags::MoveType::RAM_BALL);
			player.prio(AI::Flags::MovePrio::HIGH);
		if (player.has_ball())
				player.autochip(power);
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_goalie_random(World world) {
	Tactic::Ptr p(new PenaltyGoalieRandom(world));
	return p;
}
