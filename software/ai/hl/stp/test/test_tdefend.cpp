#include "ai/hl/hl.h"
#include "ai/hl/stp/tactic/tdefend.h"
#include "util/dprint.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/stp.h"
#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestTDefendFactory : public HighLevelFactory {
		public:
			TestTDefendFactory() : HighLevelFactory("Test STP TDefend") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestTDefendFactory factory_instance;

	class TestTDefend : public HighLevel {
		public:
			TestTDefend(World &world) : world(world) {
			}

		private:
			World &world;

			TestTDefendFactory &factory() const {
				return factory_instance;
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				tick_eval(world);

				FriendlyTeam &friendly = world.friendly_team();
				if (friendly.size() < 3) {
					return;
				}

				auto goalie = Tactic::tgoalie(world, 2);
				goalie->set_player(friendly.get(0));
				goalie->execute();

				auto defend1 = Tactic::tdefender1(world);
				defend1->set_player(friendly.get(1));
				defend1->execute();

				auto defend2 = Tactic::tdefender2(world);
				defend2->set_player(friendly.get(2));
				defend2->execute();
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_defense(world, ctx);
			}
	};

	HighLevel::Ptr TestTDefendFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestTDefend(world));
		return p;
	}
}

