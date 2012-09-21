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
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/penalty_goalie.h"
#include "ai/hl/stp/tactic/tdefend.h"
#include "geom/util.h"
#include "util/param.h"

using AI::HL::STP::PlayExecutor;

namespace Flags = AI::Flags;

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

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
	BoolParam use_simon("use simon", "MixedTeamDefense", true);
	BoolParam do_draw("draw", "MixedTeamDefense", true);

	const double RESTRICTED_ZONE_LENGTH = 1.15;

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
			std::vector<Player::Ptr> players;
			std::vector<Player::Ptr> other_players;

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
				if (!enabled[friendly.get(i)->pattern()]) {
					other_players.push_back(friendly.get(i));
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
				case AI::Common::PlayType::PREPARE_KICKOFF_ENEMY:
					default_flags |= Flags::FLAG_AVOID_BALL_STOP;
					default_flags |= Flags::FLAG_STAY_OWN_HALF;
					break;

				default:
					break;
			}

			if (players.size() > 1) {
				players[1]->flags(default_flags);
			}

			if (players.size() > 2) {
				players[2]->flags(default_flags);
			}

			switch (world.playtype()) {
				case AI::Common::PlayType::STOP:
					stop(players);
					break;

				case AI::Common::PlayType::PREPARE_PENALTY_ENEMY:
				case AI::Common::PlayType::EXECUTE_PENALTY_ENEMY:
					penalty(players);
					break;

				case AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
				case AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
					if (take_free_kick) {
						free_kick_friendly(players, other_players);
					} else {
						play(players);
					}
					break;

				default:
					play(players);
					break;
			}
		}

		void free_kick_friendly(std::vector<Player::Ptr> &players, std::vector<Player::Ptr> &other_players) {
			Action::move(world, players[0], Point(world.field().friendly_goal().x + Robot::MAX_RADIUS, 0));

			if (players.size() > 1) {
				double largest_x = -3;
				std::size_t index = 0;
				for (std::size_t i = 0; i < other_players.size(); ++i) {
					if (other_players[i]->position().x > largest_x) {
						largest_x = other_players[i]->position().x;
						index = i;
					}
				}

				// pass to the net if there are no other players, otherwise to the one farthest along the field
				Point location = world.field().enemy_goal();
				if (other_players.size() > 0) {
					location = other_players[index]->position();
				}

				Action::intercept(players[1], location);
				players[1]->autochip(1);
			}

			if (players.size() > 2) {
				auto stop2 = Tactic::move_stop(world, 2);
				stop2->set_player(players[2]);
				stop2->execute();
			}
		}

		void penalty(std::vector<Player::Ptr> &players) {
			auto goalie = Tactic::penalty_goalie(world);
			goalie->set_player(players[0]);
			goalie->execute();

			if (players.size() == 1) {
				return;
			}

			Action::move(world, players[1], Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 6 * Robot::MAX_RADIUS));

			if (players.size() == 3) {
				Action::move(world, players[2], Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -6 * Robot::MAX_RADIUS));
			}
		}

		void stop(std::vector<Player::Ptr> &players) {
			Action::move(world, players[0], Point(world.field().friendly_goal().x + Robot::MAX_RADIUS, 0));

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
		}

		void play(std::vector<Player::Ptr> &players) {
			//auto waypoints = Evaluation::evaluate_defense();
			if (players.size() == 1) {
				auto goalie = Tactic::lone_goalie(world);
				goalie->set_player(players[0]);
				goalie->execute();
				return;
			}


			if (use_simon) {
				auto goalie = Tactic::defend_duo_goalie(world);
				goalie->set_player(players[0]);
				goalie->execute();

				auto defend1 = Tactic::defend_duo_defender(world);
				defend1->set_player(players[1]);
				defend1->execute();

				if (players.size() > 2) {
					auto defend2 = Tactic::defend_duo_extra1(world);
					defend2->set_player(players[2]);
					defend2->execute();
				}

				return;
			} else {
				if (players.size() > 0) {
					auto goalie = Tactic::lone_goalie(world);
					goalie->set_player(players[0]);
					goalie->execute();
				}
				if (players.size() > 1) {
					auto defend1 = Tactic::tdefender1(world);
					defend1->set_player(players[1]);
					defend1->execute();
				}
				if (players.size() > 2) {
					auto defend2 = Tactic::tdefender2(world);
					defend2->set_player(players[2]);
					defend2->execute();
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

