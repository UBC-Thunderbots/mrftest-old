#include "ai/hl/hl.h"
#include "ai/flags.h"

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	class TestCatchBallFactory : public HighLevelFactory {
		public:
			TestCatchBallFactory() : HighLevelFactory("Test Catch Ball") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestCatchBallFactory factory_instance;

	class TestCatchBall : public HighLevel {
		public:
			TestCatchBall(World &world) : world(world) {
			}

		private:
			World &world;

			TestCatchBallFactory &factory() const {
				return factory_instance;
			}

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
				player->move(world.ball().position(), to_goal, Point());
			}
	};

	HighLevel::Ptr TestCatchBallFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestCatchBall(world));
		return p;
	}
}

