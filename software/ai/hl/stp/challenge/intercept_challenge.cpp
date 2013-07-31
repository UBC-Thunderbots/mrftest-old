#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/flags.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/action/ram.h"
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

using namespace AI::Flags;
using namespace AI::HL;
using namespace AI::HL::W;
using namespace AI::HL::STP;
using AI::HL::STP::Enemy;

using namespace AI::HL::STP::Predicates;
using AI::HL::STP::PlayExecutor;

namespace {
	// The minimum distance between the enemy baller and friendly robots as specified in regulation
	const double INTERCEPT_DIST = 0.50;

	// The closest distance enemy players allowed to the ball
	// DO NOT make this EXACT, instead, add a little tolerance!
	const double AVOIDANCE_DIST = INTERCEPT_DIST + Robot::MAX_RADIUS + Ball::RADIUS + 0.005;

	class InterceptChallenge: public HighLevel {
		public:
			InterceptChallenge(World world) :
					world(world) {
			}

		private:
			World world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				tick_eval(world);

				std::vector<AI::HL::W::Player> players = AI::HL::Util::get_players(world.friendly_team());
				
				if (world.playtype() == AI::Common::PlayType::STOP) {
					stop(players);
					return;
				}

				if (players.empty() || players.size() > 2 || world.playtype() != AI::Common::PlayType::PLAY) {
					return;
				}
				for (AI::HL::W::Player i : players) {
					i.autokick(AI::HL::STP::BALL_MAX_SPEED);
					i.flags(AI::Flags::FLAG_STAY_OWN_HALF);
				}

				const Robot baller = Evaluation::calc_enemy_baller(world);
				// if the enemy (passers) has the ball, position to block 
				if (baller && AI::HL::STP::Predicates::their_ball(world)) {
					
					Point dir_to_ball = (world.ball().position() - baller.position()).norm();
					Point target = baller.position() + (AVOIDANCE_DIST * dir_to_ball);
					Point perp_to_dir_to_ball = (world.ball().position() - baller.position()).perp().norm();

					// if there is only one enemy robot just block it...
					if (world.enemy_team().size() == 1) {
						Action::move(world, players[0], target + perp_to_dir_to_ball * 4 * Robot::MAX_RADIUS);
						Action::move(world, players[1], target - perp_to_dir_to_ball * 4 * Robot::MAX_RADIUS);
						return;
					}

					if (players.size() > 1) {
						// sort the players by dist to target
						std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(target));
						
						// 1st player blocks the baller
						Action::move(world, players[0], target);
						
						Enemy::Ptr furthest_enemy;
						if (world.enemy_team().size() > 1) {
							furthest_enemy = Enemy::closest_friendly_player(world, players[0], static_cast<unsigned int>(world.enemy_team().size() - 1));
						} 

						Robot robot = furthest_enemy->evaluate();
						Point dir_to_baller_enemy = (baller.position() - robot.position()).norm();
						Point block_target = baller.position() - (AVOIDANCE_DIST * dir_to_baller_enemy);

						// 2nd player blocks furthest robot not blocked by first player
						Action::move(world, players[1], block_target);
						
					} else if (players.size() == 1) {
						Action::move(world, players[0], target);
					}
				// else try to knock the ball out of the field
				} else if (!AI::HL::STP::Predicates::their_ball(world)) {
					ram(players);
				}
			}

			// stay away from ball
			void stop(std::vector<Player> &players){
				if (players.size() > 0) {
					auto stop1 = Tactic::move_stop(world, 2);
					stop1->set_player(players[0]);
					stop1->execute();
				}

				if (players.size() > 1) {
					auto stop2 = Tactic::move_stop(world, 3);
					stop2->set_player(players[1]);
					stop2->execute();
				}
			}

			// ram / knock the ball out of the field
			void ram(std::vector<Player> &players){
				// sort the players by dist to ball
				std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(world.ball().position()));

				// ram the ball with autokick on
				if (players.size() > 0) {
					auto ram = Tactic::ram(world);
					ram->set_player(players[0]);
					ram->execute();
				}
			}

	};
}

HIGH_LEVEL_REGISTER(InterceptChallenge)

