#include "ai/hl/hl.h"
#include "ai/flags.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/predicates.h"

using namespace AI::HL;
using namespace AI::HL::W;
using namespace AI::HL::STP;

using namespace AI::HL::STP::Predicates;

namespace {
	class InterceptChallenge : public HighLevel {
		public:
			InterceptChallenge(World &world) : world(world) {
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

				if (AI::HL::STP::Predicates::our_ball(world)){
					// do something
				}

				Player::Ptr player = friendly.get(0);
				player->autokick(AI::HL::STP::BALL_MAX_SPEED);
				Action::intercept(player, world.field().enemy_goal());

				Player::Ptr player2 = friendly.get(1);
				player2->autokick(AI::HL::STP::BALL_MAX_SPEED);
				Action::intercept(player2, world.field().enemy_goal());
			}
	};
}

HIGH_LEVEL_REGISTER(InterceptChallenge)

