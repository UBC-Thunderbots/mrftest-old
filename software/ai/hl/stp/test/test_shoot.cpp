#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/test/test.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestShootFactory : public HighLevelFactory {
		public:
			TestShootFactory() : HighLevelFactory("Test STP Shoot") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestShootFactory factory_instance;

	class TestShoot : public HighLevel {
		public:
			TestShoot(World &world) : world(world) {
			}

		private:
			World &world;

			TestShootFactory &factory() const {
				return factory_instance;
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				tick_eval(world);

				FriendlyTeam &friendly = world.friendly_team();
				if (friendly.size() == 0) {
					return;
				}

				Action::shoot_goal(world, friendly.get(0));
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_shoot(world, ctx);
			}
	};

	HighLevel::Ptr TestShootFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestShoot(world));
		return p;
	}
}

