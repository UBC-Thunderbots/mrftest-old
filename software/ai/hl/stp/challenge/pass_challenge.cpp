#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/test/test.h"
#include "ai/hl/stp/predicates.h"
#include "geom/util.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

using namespace AI::HL::STP::Predicates;
using namespace AI::Flags;
using namespace AI::HL::STP::Predicates;

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
				if (players.empty() || players.size() > 4) {
					return;
				}

				if (world.playtype() == AI::Common::PlayType::STOP){
					return stop(players);
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
			}
	};
}

HIGH_LEVEL_REGISTER(PassChallenge)

