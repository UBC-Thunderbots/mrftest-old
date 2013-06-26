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
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/penalty_goalie.h"
#include "ai/hl/stp/tactic/tdefend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/block.h"
#include "geom/util.h"
#include "util/param.h"
#include "util/dprint.h"
#include "ai/hl/stp/predicates.h"

using AI::HL::STP::PlayExecutor;

namespace Flags = AI::Flags;

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

using namespace AI::HL::STP::Predicates;

namespace {
	BoolParam enable0("enable robot 0", "MixedTeamDefense", true);
	BoolParam enable1("enable robot 1", "MixedTeamDefense", true);
	BoolParam enable2("enable robot 2", "MixedTeamDefense", true);
	BoolParam enable3("enable robot 3", "MixedTeamDefense", true);
	BoolParam enable4("enable robot 4", "MixedTeamDefense", true);
	BoolParam enable5("enable robot 5", "MixedTeamDefense", true);
	BoolParam enable6("enable robot 6", "MixedTeamDefense", true);
	BoolParam enable7("enable robot 7", "MixedTeamDefense", true);
	BoolParam enable8("enable robot 8", "MixedTeamDefense", true);
	BoolParam enable9("enable robot 9", "MixedTeamDefense", true);
	BoolParam enable10("enable robot 10", "MixedTeamDefense", true);
	BoolParam enable11("enable robot 11", "MixedTeamDefense", true);
	BoolParam take_free_kick("take the free kicks and attempt to pass to the other team", "MixedTeamDefense", false);
	BoolParam do_draw("draw", "MixedTeamDefense", true);
	
	BoolParam three_def_one_atk("3 defender 1 attacker", "MixedTeamDefense", true);
	BoolParam two_def_two_atk("2 defender 2 attacker", "MixedTeamDefense", false);

	struct MixedTeamDefense : public HighLevel {
		World world;

		MixedTeamDefense(World world) : world(world) {
		}

		HighLevelFactory &factory() const;

		Gtk::Widget *ui_controls() {
			return 0;
		}

		void tick() {
			tick_eval(world);

			FriendlyTeam friendly = world.friendly_team();
			std::vector<Player> players;
			std::vector<Player> other_players;

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
					other_players.push_back(friendly.get(i));
					continue;
				}
				players.push_back(friendly.get(i));
			}

			if (players.empty()) {
				return;
			}

			Player goalie;
			for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
				Player p = world.friendly_team().get(i);
				if (p.pattern() == world.friendly_team().goalie() && enabled[p.pattern()]) {
					goalie = p;
				}
			}

			if (!goalie) {
				LOG_ERROR("No goalie with the desired pattern!!");
			} else {
				players.push_back(goalie);
			}

			for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
				Player p = world.friendly_team().get(i);
				if (goalie && p == goalie) {
					// this is our goalie
					continue;
				} else if (!enabled[p.pattern()]) {
					other_players.push_back(friendly.get(i));
				}
				players.push_back(p);
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
				case AI::Common::PlayType::PREPARE_KICKOFF_ENEMY:
					default_flags |= Flags::FLAG_AVOID_BALL_STOP;
					default_flags |= Flags::FLAG_STAY_OWN_HALF;
					break;

				default:
					break;
			}

			if (players.size() > 1) {
				players[1].flags(default_flags);
			}

			if (players.size() > 2) {
				players[2].flags(default_flags);
			}

			if (players.size() > 3) {
				players[3].flags(default_flags);
			}

			switch (world.playtype()) {
				case AI::Common::PlayType::HALT:
					return;
				case AI::Common::PlayType::STOP:
					stop(goalie, players);
					break;

				case AI::Common::PlayType::PREPARE_PENALTY_ENEMY:
				case AI::Common::PlayType::EXECUTE_PENALTY_ENEMY:
					penalty_enemy(goalie, players);
					break;
				case AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY:
				case AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY:
					penalty_friendly(goalie, players);
					break;

				case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
				case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
					if (take_free_kick) {
						free_kick_friendly(goalie, players, other_players);
					} else {
						play(goalie, players, other_players);
					}
					break;

				default:
					play(goalie, players, other_players);
					break;
			}
		}

		void free_kick_friendly(Player goalie, std::vector<Player> &players, std::vector<Player> &other_players) {
			if (goalie) {
				Action::move(world, goalie, Point(world.field().friendly_goal().x + Robot::MAX_RADIUS, 0));
			}
			if (players.size() > 1) {
				double largest_x = -3;
				std::size_t index = 0;
				for (std::size_t i = 0; i < other_players.size(); ++i) {
					if (other_players[i].position().x > largest_x) {
						largest_x = other_players[i].position().x;
						index = i;
					}
				}

				// pass to the net if there are no other players, otherwise to the one farthest along the field
				Point location = world.field().enemy_goal();
				if (other_players.size() > 0) {
					location = other_players[index].position();
				}

				Action::intercept(players[1], location);
				players[1].autochip(1);
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

		void penalty_enemy(Player goalie, std::vector<Player> &players) {
			if (goalie) {
				auto g = Tactic::penalty_goalie(world);
				g->set_player(goalie);
				g->execute();
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

		void penalty_friendly(Player goalie, std::vector<Player> &players) {
			if (goalie) {
				auto g = Tactic::penalty_goalie(world);
				g->set_player(goalie);
				g->execute();
			}
			if (players.size() > 1) {
				Action::move(world, players[1], Point(world.field().penalty_enemy().x - DIST_FROM_PENALTY_MARK - Robot::MAX_RADIUS, 6 * Robot::MAX_RADIUS));
			}
			if (players.size() > 2) {
				Action::move(world, players[2], Point(world.field().penalty_enemy().x - DIST_FROM_PENALTY_MARK - Robot::MAX_RADIUS, -6 * Robot::MAX_RADIUS));
			}
			
			if (players.size() > 3) {
				Action::move(world, players[3], Point(world.field().penalty_enemy().x - DIST_FROM_PENALTY_MARK - Robot::MAX_RADIUS, 8 * Robot::MAX_RADIUS));
			}

		}

		void stop(Player goalie, std::vector<Player> &players) {
			if (goalie) {
				Action::move(world, goalie, Point(world.field().friendly_goal().x + Robot::MAX_RADIUS, 0));
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

		void play(Player goalie, std::vector<Player> &players, std::vector<Player> &other_players) {
			// our mixed friendly team has the ball
			bool mixed_friendly_ball = false;
			for (std::size_t i = 0; i < other_players.size(); ++i) {
				if (Evaluation::possess_ball(world, other_players[i])) {
					mixed_friendly_ball = true;
				}
			}

			if (goalie) {
				auto g = Tactic::defend_duo_goalie(world);
				g->set_player(goalie);
				g->execute();
			}
			if (players.size() > 1) {
				auto defend1 = Tactic::defend_duo_defender(world);
				defend1->set_player(players[1]);
				defend1->execute();
			}
			if (players.size() > 2) {
				if (two_def_two_atk) {
					if (!mixed_friendly_ball) {
						auto active = Tactic::shoot_goal(world, true);
						if (fight_ball(world) || their_ball(world)){
							active = Tactic::spin_steal(world);
						}
						active->set_player(players[2]);
						active->execute();
					} else {
						auto offend = Tactic::offend(world);
						offend->set_player(players[2]);
						offend->execute();
					}
				} else {
					auto defend2 = Tactic::defend_duo_extra1(world);
					defend2->set_player(players[2]);
					defend2->execute();
				}
			}

			if (players.size() > 3) {
				if (three_def_one_atk) {
					if (!mixed_friendly_ball) {
						auto active = Tactic::shoot_goal(world, true);
						if (fight_ball(world) || their_ball(world)){
							active = Tactic::spin_steal(world);
						}
						active->set_player(players[2]);
						active->execute();
					} else {
						auto offend = Tactic::offend(world);
						offend->set_player(players[3]);
						offend->execute();
					}
				} else if (two_def_two_atk) {
					auto offend2 = Tactic::offend_secondary(world);
					offend2->set_player(players[3]);
					offend2->execute();
				} else {
					auto defend3 = Tactic::defend_duo_extra2(world);
					defend3->set_player(players[3]);
					defend3->execute();
				}
			}

		}

		void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
			if (do_draw) {
				draw_ui(world, ctx);
			}
		}
	};
}

HIGH_LEVEL_REGISTER(MixedTeamDefense)

