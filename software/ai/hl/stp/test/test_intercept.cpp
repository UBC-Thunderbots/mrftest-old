#include "ai/hl/hl.h"
#include "ai/flags.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/action/intercept.h"

using namespace AI::HL;
using namespace AI::HL::W;
using namespace AI::HL::STP;

namespace {
	class TestIntercept : public HighLevel {
		public:
			explicit TestIntercept(World world) : world(world) {
			}

		private:
			World world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return nullptr;
			}

			void tick() {
				FriendlyTeam friendly = world.friendly_team();
				if (!friendly.size()) {
					return;
				}

				Player player = friendly[0];
				player.autokick(AI::HL::STP::BALL_MAX_SPEED);
				Action::intercept(player, world.field().enemy_goal());
			}
	};
}

HIGH_LEVEL_REGISTER(TestIntercept)

