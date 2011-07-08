#include "ai/hl/hl.h"
#include "ai/hl/stp/action/chase.h"
#include "util/dprint.h"
#include "ai/hl/stp/action/move_spin.h"
#include "ai/hl/stp/stp.h"

#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestSpinFactory : public HighLevelFactory {
		public:
			TestSpinFactory() : HighLevelFactory("Test STP Spin") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestSpinFactory factory_instance;

	class TestSpin : public HighLevel {
		public:
			TestSpin(World &world) : world(world) {
			}

		private:
			World &world;

			TestSpinFactory &factory() const {
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
				Point dirToBall = (world.ball().position() - friendly.get(0)->position()).norm();
				Action::move_spin(friendly.get(0), world.ball().position() + Robot::MAX_RADIUS * dirToBall);
			}
	};

	HighLevel::Ptr TestSpinFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestSpin(world));
		return p;
	}
}

