#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/penalty_goalie.h"
#include "ai/hl/stp/tactic/tdefend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/free_kick_pass.h"
#include "ai/hl/stp/predicates.h"
#include "geom/util.h"
#include "util/param.h"

using AI::HL::STP::PlayExecutor;

namespace Flags = AI::Flags;

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;
using namespace AI::HL::STP::Predicates;

namespace {
	BoolParam enable0("enable robot 0", "MixedTeamOffense", true);
	BoolParam enable1("enable robot 1", "MixedTeamOffense", true);
	BoolParam enable2("enable robot 2", "MixedTeamOffense", true);
	BoolParam enable3("enable robot 3", "MixedTeamOffense", true);
	BoolParam enable4("enable robot 4", "MixedTeamOffense", true);
	BoolParam enable5("enable robot 5", "MixedTeamOffense", true);
	BoolParam enable6("enable robot 6", "MixedTeamOffense", true);
	BoolParam enable7("enable robot 7", "MixedTeamOffense", true);
	BoolParam enable8("enable robot 8", "MixedTeamOffense", true);
	BoolParam enable9("enable robot 9", "MixedTeamOffense", true);
	BoolParam enable10("enable robot 10", "MixedTeamOffense", true);
	BoolParam enable11("enable robot 11", "MixedTeamOffense", true);
	BoolParam do_draw("draw", "MixedTeamOffense", true);

	struct MixedTeamOffense : public HighLevel {
		World world;

		MixedTeamOffense(World world) : world(world) {
		}

		HighLevelFactory &factory() const;

		Gtk::Widget *ui_controls() {
			return 0;
		}

		void tick() {
			tick_eval(world);

			FriendlyTeam friendly = world.friendly_team();
			std::vector<Player> players;

			const bool enabled[] = {
				enable0,
				enable1,
				enable2,
				enable3,
				enable4,
				enable5,
				enable6,
				enable7,
				enable8,
				enable9,
				enable10,
				enable11,
			};

			for (std::size_t i = 0; i < friendly.size(); ++i) {
				if (!enabled[friendly.get(i).pattern()]) {
					continue;
				}
				players.push_back(friendly.get(i));
			}

			if (players.empty()) {
				return;
			}

			unsigned int default_flags = Flags::FLAG_AVOID_FRIENDLY_DEFENSE;
			switch (world.playtype()) {
				case AI::Common::PlayType::STOP:
				case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
				case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
					default_flags |= Flags::FLAG_AVOID_BALL_STOP;
					break;

				case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
				case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
					default_flags |= Flags::FLAG_AVOID_ENEMY_DEFENSE;
					break;

				case AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY:
				case AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY:
				case AI::Common::PlayType::PREPARE_KICKOFF_ENEMY:
				case AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY:
					default_flags |= Flags::FLAG_AVOID_BALL_STOP;
					default_flags |= Flags::FLAG_STAY_OWN_HALF;
					break;
				case AI::Common::PlayType::PREPARE_PENALTY_ENEMY:
				case AI::Common::PlayType::EXECUTE_PENALTY_ENEMY:
					default_flags |= Flags::FLAG_AVOID_BALL_STOP;
					default_flags |= Flags::FLAG_AVOID_ENEMY_DEFENSE;
					default_flags |= Flags::FLAG_PENALTY_KICK_ENEMY;

				default:
					break;
			}
			
			for (std::size_t i = 0 ; i < players.size() ; i++) {
				players[i].flags(default_flags);
			}

			switch (world.playtype()) {
				case AI::Common::PlayType::HALT:
					return;
				case AI::Common::PlayType::STOP:
					stop(players);
					break;
				case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:	
				case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
					free_kick_enemy(players);
					break;	
				case AI::Common::PlayType::PREPARE_PENALTY_ENEMY:
				case AI::Common::PlayType::EXECUTE_PENALTY_ENEMY:
					penalty_enemy(players);
					break;
				case AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY:
					prepare_penalty_friendly(players);
					break;	
				case AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY:
					execute_penalty_friendly(players);
					break;	
				case AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY:
					prepare_kickoff_friendly(players);
					break;
				case AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY:
					execute_kickoff_friendly(players);
					break;
				case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:	
				case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
					free_kick_friendly(players);
					break;
				case AI::Common::PlayType::PLAY:
					play(players);
				default:
					return;
					break;
			}
		}

		void free_kick_enemy(std::vector<Player> &players) {
			if (players.size() > 0) {
				auto shadow = Tactic::shadow_ball(world);
				shadow->set_player(players[0]);
				shadow->execute();
			}

			if (players.size() > 1) {
				auto stop1 = Tactic::move_stop(world, 1);
				stop1->set_player(players[1]);
				stop1->execute();
			}

			if (players.size() > 2) {
				auto stop2 = Tactic::move_stop(world, 2);
				stop2->set_player(players[2]);
				stop2->execute();
			}

			if (players.size() >3) {
				auto stop3 = Tactic::move_stop(world, 3);
				stop3->set_player(players[3]);
				stop3->execute();
			}
		}

		void penalty_enemy(std::vector<Player> &players) {
			if (players.size() > 0){
				Action::move(world, players[0], Point(world.field().penalty_friendly().x + DIST_FROM_PENALTY_MARK + Robot::MAX_RADIUS, 6 * Robot::MAX_RADIUS));
			}
			if (players.size() > 1) {
				Action::move(world, players[1], Point(world.field().penalty_friendly().x + DIST_FROM_PENALTY_MARK + 4 * Robot::MAX_RADIUS, 0 * Robot::MAX_RADIUS));
			}
			if (players.size() > 2) {
				Action::move(world, players[2], Point(world.field().penalty_friendly().x + DIST_FROM_PENALTY_MARK + Robot::MAX_RADIUS, 6 * Robot::MAX_RADIUS));
			}
			//check magic numbers. what do they mean?
			if (players.size() > 3) {
				Action::move(world, players[3], Point(world.field().penalty_friendly().x + DIST_FROM_PENALTY_MARK + Robot::MAX_RADIUS, 8 * Robot::MAX_RADIUS));
			}
		}

		void prepare_penalty_friendly(std::vector<Player> &players) {
			if (players.size() > 0) {
				Action::move(world, players[0], Point(world.field().penalty_enemy().x - Robot::MAX_RADIUS, 0));
			}
			if (players.size() > 1) {
				Action::move(world, players[1], Point(world.field().penalty_enemy().x - DIST_FROM_PENALTY_MARK - Robot::MAX_RADIUS, 6 * Robot::MAX_RADIUS));
			}
			if (players.size() > 2) {
				Action::move(world, players[2], Point(world.field().penalty_enemy().x - DIST_FROM_PENALTY_MARK - Robot::MAX_RADIUS, -6 * Robot::MAX_RADIUS));
			}
			//again clarify on magic numbers!
			if (players.size() > 3) {
				Action::move(world, players[3], Point(world.field().penalty_enemy().x - DIST_FROM_PENALTY_MARK - Robot::MAX_RADIUS, 8 * Robot::MAX_RADIUS));
			}

		}
		
		void execute_penalty_friendly(std::vector<Player> &players) {
			if (players.size() > 0) {
				auto shooter = Tactic::penalty_shoot(world);
				shooter->set_player(players[0]);
				shooter->execute();
			}
			if (players.size() > 1) {
				Action::move(world, players[1], Point(world.field().penalty_enemy().x - DIST_FROM_PENALTY_MARK - Robot::MAX_RADIUS, 6 * Robot::MAX_RADIUS));
			}
			if (players.size() > 2) {
				Action::move(world, players[2], Point(world.field().penalty_enemy().x - DIST_FROM_PENALTY_MARK - Robot::MAX_RADIUS, -6 * Robot::MAX_RADIUS));
			}
			//magic number!!
			if (players.size() > 3) {
				Action::move(world, players[3], Point(world.field().penalty_enemy().x - DIST_FROM_PENALTY_MARK - Robot::MAX_RADIUS, 8 * Robot::MAX_RADIUS));
			}

		}

		void prepare_kickoff_friendly(std::vector<Player> &players) {
			// the distance we want the players to the ball
			const double AVOIDANCE_DIST = Ball::RADIUS + Robot::MAX_RADIUS + 0.005;

			// distance for the offenders to be positioned away from the kicker
			const double SEPARATION_DIST = 10 * Robot::MAX_RADIUS;

			// hard coded positions for the kicker, and 2 offenders
			Point kicker_position = Point(-0.5 - Ball::RADIUS - Robot::MAX_RADIUS, 0);
			Point ready_positions[3] = { Point(-AVOIDANCE_DIST, -SEPARATION_DIST), Point(-AVOIDANCE_DIST, SEPARATION_DIST), Point(AVOIDANCE_DIST, SEPARATION_DIST) };

			// sort the players by dist to ball
			//std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(world.ball().position()));

			if (players.size() > 0) {
				Action::move(world, players[0], kicker_position);
			}
			if (players.size() > 1) {
				Action::move(world, players[1], ready_positions[0]);
			} 
			if (players.size() > 2) {
				Action::move(world, players[2], ready_positions[1]);
			}
			if (players.size() > 3) {
				Action::move(world, players[3], ready_positions[2]);
			}

		}
		
		void execute_kickoff_friendly(std::vector<Player> &players) {
			// the distance we want the players to the ball
			const double AVOIDANCE_DIST = Ball::RADIUS + Robot::MAX_RADIUS + 0.005;

			// distance for the offenders to be positioned away from the kicker
			const double SEPARATION_DIST = 10 * Robot::MAX_RADIUS;

			// hard coded positions for the kicker, and 2 offenders
			//Point kicker_position = Point(-0.5 - Ball::RADIUS - Robot::MAX_RADIUS, 0);
			Point ready_positions[3] = { Point(-AVOIDANCE_DIST, -SEPARATION_DIST), Point(-AVOIDANCE_DIST, SEPARATION_DIST), Point(AVOIDANCE_DIST, SEPARATION_DIST) };

			// sort the players by dist to ball
			//std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(world.ball().position()));

			if (players.size() > 0) {
				Action::intercept(players[0], world.field().enemy_goal());
				if (players[0].has_chipper()) {
					players[0].autochip(1);
				}
			}
			if (players.size() > 1) {
				Action::move(world, players[1], ready_positions[0]);
			} 
			if (players.size() > 2) {
				Action::move(world, players[2], ready_positions[1]);
			}
			if (players.size() > 3) {
				Action::move(world, players[3], ready_positions[2]);
			}

		}

		void free_kick_friendly(std::vector<Player> &players) {
			// sort the players by dist to ball
			//std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(world.ball().position()));
			if (players.size() > 0) {
				Action::intercept(players[0], world.field().enemy_goal());
				if (players[0].has_chipper()) {
					players[0].autochip(1);
				}
			}

			if (players.size() > 1) {
				auto stop1 = Tactic::move_stop(world, 1);
				stop1->set_player(players[1]);
				stop1->execute();
			}

			if (players.size() > 2) {
				auto stop2 = Tactic::move_stop(world, 2);
				stop2->set_player(players[2]);
				stop2->execute();
			}
			if (players.size() > 3) {
				auto stop3 = Tactic::move_stop(world, 3);
				stop3->set_player(players[3]);
				stop3->execute();
			}

		}

		void stop(std::vector<Player> &players) {
			if (players.size() > 0) {
				auto stop = Tactic::move_stop(world, 2);
				stop->set_player(players[0]);
				stop->execute();
			}

			if (players.size() > 1) {
				auto stop1 = Tactic::move_stop(world, 3);
				stop1->set_player(players[1]);
				stop1->execute();
			}

			if (players.size() > 2) {
				auto stop2 = Tactic::move_stop(world, 4);
				stop2->set_player(players[2]);
				stop2->execute();
			}
			if (players.size() > 3) {
				auto stop2 = Tactic::move_stop(world, 5);
				stop2->set_player(players[3]);
				stop2->execute();
			}

		}

		void play(std::vector<Player> &players) {
			// sort the players by dist to ball
			std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(world.ball().position()));
			if (players.size() > 0) {
				auto active = Tactic::shoot_goal(world, true);
				if (fight_ball(world) || their_ball(world)){
					active = Tactic::spin_steal(world);
				}
				active->set_player(players[0]);
				active->execute();
			}

			if (players.size() > 1) {
				auto offend = Tactic::offend(world);
				offend->set_player(players[1]);
				offend->execute();
			}
			if (players.size() > 2) {
				auto block = Tactic::block_ball(world, Enemy::closest_ball(world, 1));
				block->set_player(players[2]);
				block->execute();
			}
			if (players.size() > 3) {
				auto block2 = Tactic::block_ball(world, Enemy::closest_ball(world, 2));
				block2->set_player(players[3]);
				block2->execute();
			}			

		}

		void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
			if (do_draw) {
				draw_ui(world, ctx);
			}
		}
	};
}

HIGH_LEVEL_REGISTER(MixedTeamOffense)

