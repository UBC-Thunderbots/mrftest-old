#include "ai/hl/hl.h"
#include "ai/flags.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/ball.h"

using namespace AI::HL;
using namespace AI::HL::W;
using namespace AI::HL::STP;

using namespace AI::HL::STP::Predicates;

namespace {
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

		Player::Ptr player2 = friendly.get(1);
		player2->autokick(AI::HL::STP::BALL_MAX_SPEED);

		if (AI::HL::STP::Predicates::their_ball(world)) {
			//const Robot::Ptr baller = Evaluation::calc_enemy_baller(world);


		} else if (!AI::HL::STP::Predicates::their_ball(world)
				&& !AI::HL::STP::Predicates::our_ball(world)) {
			Action::ram(world, player);
			Action::ram(world, player2);
		}

	}
};
}

HIGH_LEVEL_REGISTER(InterceptChallenge)

