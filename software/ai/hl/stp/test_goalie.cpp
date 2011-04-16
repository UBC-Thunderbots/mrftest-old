#include "ai/hl/hl.h"
#include "ai/hl/stp/action/goalie.h"
#include "util/dprint.h"

#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestLoneGoalieFactory : public HighLevelFactory {
		public:
			TestLoneGoalieFactory() : HighLevelFactory("Test STP Lone Goalie") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestLoneGoalieFactory factory_instance;

	class TestLoneGoalie : public HighLevel {
		public:
			TestLoneGoalie(World& world) : world(world) {
			}

		private:
			World& world;

			TestLoneGoalieFactory &factory() const {
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

				Action::lone_goalie(world, friendly.get(0));
			}
	};

	HighLevel::Ptr TestLoneGoalieFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestLoneGoalie(world));
		return p;
	}
}

