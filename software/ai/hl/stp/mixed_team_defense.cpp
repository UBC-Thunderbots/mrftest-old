#include "ai/hl/hl.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/defend.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/penalty_goalie.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "util/dprint.h"
#include "ai/hl/stp/ui.h"
#include "util/param.h"

#include <cassert>
#include <gtkmm.h>

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
	
	const double RESTRICTED_ZONE_LENGTH = 0.85;

	class MixedTeamDefenseFactory : public HighLevelFactory {
		public:
			MixedTeamDefenseFactory() : HighLevelFactory("Mixed Team Defense") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	MixedTeamDefenseFactory factory_instance;

	struct MixedTeamDefense : public HighLevel {

		World &world;

		MixedTeamDefense(World &world) : world(world) {
		}

		MixedTeamDefenseFactory &factory() const {
			return factory_instance;
		}

		Gtk::Widget *ui_controls() {
			return 0;
		}

		void tick() {
			FriendlyTeam &friendly = world.friendly_team();
			std::vector<Player::Ptr> players;

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
			};

			for (std::size_t i = 0; i < friendly.size(); ++i) {
				if (!enabled[i]) continue;
				players.push_back(friendly.get(i));
			}

			if (players.size() == 0) {
				return;
			}

			switch (world.playtype()) {
				case AI::Common::PlayType::STOP:
					stop(players);
					break;
				case AI::Common::PlayType::PREPARE_PENALTY_ENEMY:
				case AI::Common::PlayType::EXECUTE_PENALTY_ENEMY:
					penalty(players);
					break;
				default:
					play(players);
					break;
			}
		}

		void penalty(std::vector<Player::Ptr>& players) {
			if (players.size() == 1) {
				auto goalie = Tactic::penalty_goalie(world);
				goalie->set_player(players[0]);
				goalie->execute();
				return;
			}

			auto goalie = Tactic::penalty_goalie(world);
			goalie->set_player(players[0]);
			goalie->execute();

			Action::move(world, players[1], Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS));

			if (players.size() == 3) {
				Action::move(world, players[2], Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 2 * Robot::MAX_RADIUS));
			}
		
		}

		void stop(std::vector<Player::Ptr>& players) {
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

		void play(std::vector<Player::Ptr>& players) {

			auto waypoints = Evaluation::evaluate_defense(world);

			if (players.size() == 1) {
				auto goalie = Tactic::lone_goalie(world);
				goalie->set_player(players[0]);
				goalie->execute();
				return;
			}

			auto goalie = Tactic::defend_duo_goalie(world);
			goalie->set_player(players[0]);
			goalie->execute();

			auto defend1 = Tactic::defend_duo_defender(world);
			defend1->set_player(players[1]);
			defend1->execute();

			if (players.size() == 3) {
				auto defend2 = Tactic::defend_duo_extra1(world);
				defend2->set_player(players[2]);
				defend2->execute();
			}
		}
	};

	HighLevel::Ptr MixedTeamDefenseFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new MixedTeamDefense(world));
		return p;
	}
}

