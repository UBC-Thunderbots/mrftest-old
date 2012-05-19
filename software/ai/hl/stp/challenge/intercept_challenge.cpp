#include "ai/hl/hl.h"
#include "ai/flags.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/action/move.h"
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

namespace {
	// The minimum distance between the baller and friendly robots as specified in regulation
	const double INTERCEPT_DIST = 0.50;

	// The closest distance players allowed to the ball
	// DO NOT make this EXACT, instead, add a little tolerance!
	const double AVOIDANCE_DIST = INTERCEPT_DIST + Robot::MAX_RADIUS
			+ Ball::RADIUS + 0.005;

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
				FriendlyTeam &friendly = world.friendly_team();
				if (friendly.size() != 2) {
					return;
				}

				Player::Ptr player = friendly.get(0);
				player->autokick(AI::HL::STP::BALL_MAX_SPEED);
				player->flags(AI::Flags::FLAG_STAY_OWN_HALF);

				Player::Ptr player2 = friendly.get(1);
				player2->autokick(AI::HL::STP::BALL_MAX_SPEED);
				player2->flags(AI::Flags::FLAG_STAY_OWN_HALF);

				if (AI::HL::STP::Predicates::their_ball(world)) {
					const Robot::Ptr baller = Evaluation::calc_enemy_baller(
							world);
					Point dirToBall = (world.ball().position() - baller->position()).norm();
					Point target = baller->position() + (0.50 * Robot::MAX_RADIUS * dirToBall);
					Point perpToDirToBall = (world.ball().position() - baller->position()).perp();
					Action::move(world, player, target + perpToDirToBall * Robot::MAX_RADIUS*2);
					Action::move(world, player, target + perpToDirToBall * -Robot::MAX_RADIUS*2);
				} else if (!AI::HL::STP::Predicates::their_ball(world)
						&& !AI::HL::STP::Predicates::our_ball(world)) {
					Action::ram(world, player);
					Action::ram(world, player2);
				} else if (AI::HL::STP::Predicates::our_ball(world)) {
					Action::ram(world, player);
					Action::ram(world, player2);
				}

			}
	};
}

HIGH_LEVEL_REGISTER(InterceptChallenge)

