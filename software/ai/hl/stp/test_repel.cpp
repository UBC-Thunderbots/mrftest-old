#include "ai/hl/hl.h"
#include "ai/hl/stp/action/chase.h"
#include "util/dprint.h"
#include "ai/hl/stp/action/repel.h"
#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestRepelFactory : public HighLevelFactory {
		public:
			TestRepelFactory() : HighLevelFactory("Test STP Repel") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestRepelFactory factory_instance;

	class TestRepel : public HighLevel {
		public:
			TestRepel(World &world) : world(world) {
			}

		private:
			World &world;

			TestRepelFactory &factory() const {
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
				
				Action::repel(world, friendly.get(0));
			}
	};

	HighLevel::Ptr TestRepelFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestRepel(world));
		return p;
	}
}

