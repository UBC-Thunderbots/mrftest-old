#include "ai/hl/hl.h"
#include "ai/flags.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "geom/point.h"

using namespace AI::Flags;
using namespace AI::HL;
using namespace AI::HL::W;
using namespace AI::HL::STP;

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
			InterceptChallenge(World &world) :
					world(world) {
			}

		private:
			World &world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				tick_eval(world);

				FriendlyTeam &friendly = world.friendly_team();
				std::vector<Player::Ptr> players;

				for (std::size_t i = 0; i < friendly.size(); ++i) {
					players.push_back(friendly.get(i));
				}

				if (players.empty() || players.size() > 2) {
					return;
				}
				
				if (world.playtype() == AI::Common::PlayType::STOP){
					stop(players);
					return;
				}

				players[0]->autokick(AI::HL::STP::BALL_MAX_SPEED);
				players[0]->flags(AI::Flags::FLAG_STAY_OWN_HALF);

				players[1]->autokick(AI::HL::STP::BALL_MAX_SPEED);
				players[1]->flags(AI::Flags::FLAG_STAY_OWN_HALF);

				if (AI::HL::STP::Predicates::their_ball(world)) {
					const Robot::Ptr baller = Evaluation::calc_enemy_baller(world);
					Point dirToBall = (world.ball().position() - baller->position()).norm();
					Point target = baller->position() + (AVOIDANCE_DIST * dirToBall);
					Point perpToDirToBall = (world.ball().position() - baller->position()).perp().norm();
					Action::move(world, players[0], target + perpToDirToBall * 4 * Robot::MAX_RADIUS);
					Action::move(world, players[1], target - perpToDirToBall * 4 * Robot::MAX_RADIUS);
				} else if (!AI::HL::STP::Predicates::their_ball(world) && !AI::HL::STP::Predicates::our_ball(world)) {
					Action::ram(world, players[0]);
					Action::ram(world, players[1]);
				} else if (AI::HL::STP::Predicates::our_ball(world)) {
					Action::ram(world, players[0]);
					Action::ram(world, players[1]);
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
			}
	};
}

HIGH_LEVEL_REGISTER(InterceptChallenge)

