#include "ai/hl/hl.h"
#include "ai/hl/stp/action/chase.h"
#include "util/dprint.h"

#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestChaseFactory : public HighLevelFactory {
		public:
			TestChaseFactory() : HighLevelFactory("Test STP Chase") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestChaseFactory factory_instance;

	class TestChase : public HighLevel {
		public:
			TestChase(World& world) : world(world) {
			}

		private:
			World& world;

			TestChaseFactory &factory() const {
				return factory_instance;
			}

			Gtk::Widget *ui_controls() {
				return NULL;
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context>) {
			}

			void tick() {
				FriendlyTeam& friendly = world.friendly_team();
				if (friendly.size() == 0) return;

				Action::chase(world, friendly.get(0));
			}
	};

	HighLevel::Ptr TestChaseFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestChase(world));
		return p;
	}
}

