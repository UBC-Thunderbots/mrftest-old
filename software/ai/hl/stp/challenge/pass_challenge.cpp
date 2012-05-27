#include "ai/util.h"
#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/flags.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/tactic/intercept.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/ram.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "geom/point.h"

#include <iostream>
using namespace std;

using namespace AI::Flags;
using namespace AI::HL;
using namespace AI::HL::W;
using namespace AI::HL::STP;
using AI::HL::STP::Enemy;

using namespace AI::HL::STP::Predicates;
using AI::HL::STP::PlayExecutor;

namespace {
	
	// The closest distance players are allowed to the ball
	// DO NOT make this EXACT, instead, add a little tolerance!
	const double AVOIDANCE_DIST = 1.0 + Robot::MAX_RADIUS + Ball::RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const Angle AVOIDANCE_ANGLE = 2.0 * Angle::of_radians(std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST));

	// how far apart the passees should be separated
	DegreeParam pass_separation_angle("CHALLENGE: angle to separate players (degrees)", "STP/Challenge", 120.0, 0.0, 180.0);
	// how fast the baller should spin
	RadianParam baller_spin_delta("CHALLENGE: change in orientation every time tick for move spin (radians)", "STP/Challenge", 3.0, 1.0, 5.0);

	class PassChallenge : public HighLevel {
		public:
			PassChallenge(World &world) : world(world) {
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

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				tick_eval(world);

				std::vector<AI::HL::W::Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
				
				if (world.playtype() == AI::Common::PlayType::STOP){
					return stop(players);
				}

				if (players.empty() || players.size() > 4 || world.playtype() != AI::Common::PlayType::PLAY) {
					return;
				}

				for (std::size_t i = 0; i < players.size(); ++i) {
					players[i]->flags(AI::Flags::FLAG_STAY_OWN_HALF);
				}

				const Player::CPtr baller = Evaluation::calc_friendly_baller();
				if (baller && AI::HL::STP::Predicates::our_ball(world)){
					
					// sort the players by dist to ball
					std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));

					Point dirToBall = (world.ball().position() - baller->position()).norm();
					Point target = baller->position() + (AVOIDANCE_DIST * dirToBall);

					const Angle delta_angle = AVOIDANCE_ANGLE + AI::HL::STP::Tactic::separation_angle;
					Point ball_pos = world.ball().position();
					Point ray = (baller->position() - ball_pos).norm();
					Point start = ball_pos + ray * AVOIDANCE_DIST;

					const Point shoot = (start - ball_pos);

					// look for a friendly player that is good to pass
					if (AI::HL::STP::Predicates::baller_can_pass(world)) {
						//players[0]->autokick(6.0); // might want to autochip?
						// everybody else goes towards where ball is likely to go lol
						int w = 1;
						for (std::size_t i = 1; i < players.size(); ++i) {
							Angle angle = delta_angle * (w / 2) * ((w % 2) ? 1 : -1);
							Point p = ball_pos - shoot.rotate(angle);
							w++;
							Action::move(world, players[i], AI::HL::Util::crop_point_to_field(world.field(),p));
						}
					} else {						
						// player with the ball turns around while trying to move to center of the half field
						players[0]->move(Point(-world.field().length()/4, 0.0), (players[0]->orientation() + baller_spin_delta).angle_mod(), Point());

						// everybody else turns with the baller
						int w = 1;
						Angle pass_angle = (AVOIDANCE_ANGLE + pass_separation_angle);
						for (std::size_t i = 1; i < players.size(); ++i) {
							Angle angle = pass_angle * (w / 2) * ((w % 2) ? 1 : -1);
							Point p = ball_pos - shoot.rotate(angle); 
							w++;
							Action::move(world, players[i], AI::HL::Util::crop_point_to_field(world.field(),p));
						}
					}
					

				// grab / intercept the ball
				} else {
					intercept_block(players);	
				}
			}

			void intercept_block(std::vector<Player::Ptr> &players){
				// dunno if it's a good idea to have 2 players grab the ball...

				// sort the players by dist to ball
				std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));

				if (players.size() > 0) {
					auto intercept1 = Tactic::intercept(world);
					intercept1->set_player(players[0]);
					intercept1->execute();
				}
				if (players.size() > 1) {
					auto intercept2 = Tactic::intercept(world);
					intercept2->set_player(players[1]);
					intercept2->execute();
				}
				// block the enemies that are closest to ball
				if (players.size() > 2 && world.enemy_team().size() > 0) {
					auto block1 = Tactic::block_ball(world, Enemy::closest_ball(world, 0));
					block1->set_player(players[2]);
					block1->execute();
				}

				if (players.size() > 3 && world.enemy_team().size() > 1) {
					auto block2 = Tactic::block_ball(world, Enemy::closest_ball(world, 1));
					block2->set_player(players[3]);
					block2->execute();
				}
			}

			void stop(std::vector<Player::Ptr> &players){
				// first player should grab the ball or not in STOP? 
				if (players.size() > 0) {
					auto stop1 = Tactic::move_stop(world, 1);
					stop1->set_player(players[0]);
					stop1->execute();
				}
				if (players.size() > 1) {
					auto stop2 = Tactic::move_stop(world, 2);
					stop2->set_player(players[1]);
					stop2->execute();
				}
				if (players.size() > 2) {
					auto stop3 = Tactic::move_stop(world, 3);
					stop3->set_player(players[2]);
					stop3->execute();
				}
				if (players.size() > 3) {
					auto stop4 = Tactic::move_stop(world, 4);
					stop4->set_player(players[3]);
					stop4->execute();
				}
			}
	};
}

HIGH_LEVEL_REGISTER(PassChallenge)

