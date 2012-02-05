#include "ai/hl/hl.h"
#include "ai/flags.h"

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	class TestCatchBall : public HighLevel {
		public:
			TestCatchBall(World &world) : world(world) {
			}

		private:
			World &world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();
				if (friendly.size() == 0) {
					return;
				}

				Player::Ptr player = friendly.get(0);
				Angle to_goal = (Point(world.field().length(), 0) - player->position()).orientation();
				player->type(AI::Flags::MoveType::INTERCEPT);
				player->move(world.field().enemy_goal(), to_goal, Point());
			}
	};
}

HIGH_LEVEL_REGISTER(TestCatchBall)

