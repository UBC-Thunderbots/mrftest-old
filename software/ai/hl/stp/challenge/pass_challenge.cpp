#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/flags.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/action/move.h"
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

				//const Player::CPtr baller = Evaluation::calc_friendly_baller();

				players[0]->flags(AI::Flags::FLAG_STAY_OWN_HALF);
				players[1]->flags(AI::Flags::FLAG_STAY_OWN_HALF);
				players[2]->flags(AI::Flags::FLAG_STAY_OWN_HALF);
				players[3]->flags(AI::Flags::FLAG_STAY_OWN_HALF);

				if (AI::HL::STP::Predicates::our_ball(world)){
					// do something
				}
			}

			void stop(std::vector<Player::Ptr> &players){
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

