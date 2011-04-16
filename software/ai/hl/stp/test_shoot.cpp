#include "ai/hl/hl.h"
#include "ai/hl/stp/action/shoot.h"
#include "util/dprint.h"

#include <cassert>
#include <gtkmm.h>

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
			TestShoot(World& world) : world(world) {
			}

		private:
			World& world;

			TestShootFactory &factory() const {
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

				Action::shoot(world, friendly.get(0));
			}
	};

	HighLevel::Ptr TestShootFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestShoot(world));
		return p;
	}
}

