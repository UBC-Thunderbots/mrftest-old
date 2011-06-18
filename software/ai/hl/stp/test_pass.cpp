#include "ai/hl/hl.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/move.h"
#include "geom/util.h"
#include "util/dprint.h"

#include <ctime>
#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {

	IntParam pass_target("passing target points", "STP/test_pass", 0, 0, 14);
	DoubleParam negligible_velocity("velocity to ignore", "STP/test_pass", 0.1, 0.0, 1.0);
	
	DoubleParam passer_tol_target(" angle tolerance that the passer needs to be with respect to the target", "STP/test_pass", 30.0, 0.0, 180.0);
	DoubleParam passer_tol_reciever(" angle tolerance that the passer needs to be with respect to the passee", "STP/test_pass", 20.0, 0.0, 180.0);	
	DoubleParam passee_tol(" distance tolerance that the passee needs to be with respect to the passer shot", "STP/test_pass", 0.05, 0.0, 1.0);
	
	// make better targets

	const Point default_targets[] = {
		Point(1.2, 0),
		Point(1.5, 0),
		Point(1.2, 0.3),
		Point(1.2, -0.3),
		Point(1.2, 0),
		Point(1.2, -0.3),
		Point(1.2, 0),
		Point(0.5, 0),
		Point(2.5, 0),
		Point(0.5, 1.2),
		Point(1, -0.6),
		Point(2, 0.6),
		Point(1, -0.6),
		Point(0.5, 0),
		Point(2.5, 0.6)
	};

	const int default_targets_n = G_N_ELEMENTS(default_targets);

	class TestPassFactory : public HighLevelFactory {
		public:
			TestPassFactory() : HighLevelFactory("Test STP Pass") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestPassFactory factory_instance;

	class TestPass : public HighLevel {
		public:
			TestPass(World &world) : world(world), targets(default_targets, default_targets + default_targets_n) {
				kicked_count = 0;
				kicked = false;
			}

		private:
			World &world;

			std::vector<Point> targets;
			
			// the position of the passing robot before/during the kick
			Point player_kick_pos;

			bool kicked;
			
			int kicked_count;
			
			TestPassFactory &factory() const {
				return factory_instance;
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				std::vector<AI::HL::W::Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
				if (players.size() != 2) {
					return;
				}

	//			std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
		//		std::reverse(players.begin(), players.end());
				
				if (kicked) {
					kicked_count++;
				} else {
					kicked_count = 0;
				}
				if (kicked_count > 50) {
					kicked = false;
				}
				
				
				
				
				
/*

				if(!Action::within_pivot_thresh(world, players[0], targets[pass_target])){
					Action::chase(world, players[0], targets[pass_target]);
					Action::move(players[1], (world.ball().position() - players[1]->position()).orientation(), targets[pass_target]);
					return;
				}

				if(!Action::within_angle_thresh(players[0], targets[pass_target], passer_tol_target)){
					Action::chase_pivot(world,players[0], targets[pass_target]);
					Action::move(players[1], (world.ball().position() - players[1]->position()).orientation(), targets[pass_target]);
				}else{
					Action::move(players[0], players[1]->position().orientation(), players[1]->position());
					Point passer_dir(100, 0);
					passer_dir = passer_dir.rotate(players[0]->orientation());					
					Point intercept_pos = closest_lineseg_point(players[1]->position(), world.ball().position(), players[0]->position() + passer_dir);
					Action::move(players[1], (world.ball().position() - players[1]->position()).orientation(), targets[pass_target]);
				}


*/

if(kicked){

Action::move(players[0], players[0]->orientation(), players[0]->position());

}else if (!players[0]->has_ball()) {
					Action::chase_pivot(world,players[0], targets[pass_target]);
					//Action::chase(world, players[0], targets[pass_target]);
				}
		

			// passer grabs ball
				
				// passer shoots					
				if (players[0]->has_ball()) {
				//	if(!kicked){
						if (Action::shoot_target(world, players[0], targets[pass_target], true)) { 
							kicked = true;
						}
				//	}
				}
								
				bool fast_ball = world.ball().velocity().len() > negligible_velocity;
							
				if(kicked && fast_ball){
				
					Point pass_dir(100, 0);
					pass_dir = pass_dir.rotate(players[0]->orientation());
					
					Point intercept_pos = closest_lineseg_point(players[1]->position(), players[0]->position(), players[0]->position() + pass_dir);
					//double intercept_ori = (world.ball().position() - intercept_pos).orientation();
					Action::move(players[1], (players[0]->position() - intercept_pos).orientation(), intercept_pos);
					//Action::chase(world, players[1]);
				} else {
					// passee move to target
					Action::move(players[1], (world.ball().position() - players[1]->position()).orientation(), targets[pass_target]);
				}

				// passee grabs ball
				//if (kicked) Action::chase(world, players[1]);
				players[1]->type(AI::Flags::MoveType::DRIBBLE);
				
			}
	};

	HighLevel::Ptr TestPassFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestPass(world));
		return p;
	}
}

