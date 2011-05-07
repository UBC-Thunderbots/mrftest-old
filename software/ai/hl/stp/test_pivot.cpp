#include "ai/hl/hl.h"
#include "ai/hl/stp/action/pivot.h"
#include "util/dprint.h"

#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestPivotFactory : public HighLevelFactory {
		public:
			TestPivotFactory() : HighLevelFactory("Test STP Pivot") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestPivotFactory factory_instance;

	class TestPivot : public HighLevel {
		public:
			TestPivot(World& world) : world(world) {
			}

		private:
			World& world;
			bool target_enemy;

			TestPivotFactory &factory() const {
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

				Player::Ptr player = friendly.get(0);

				if (fabs(player->orientation()) < 1e-3) {
					target_enemy = true;
				} else if (fabs(player->orientation() - M_PI) < 1e-3
					|| fabs(player->orientation() + M_PI) < 1e-3) {
					target_enemy = false;
				}

				Point target;

				if (target_enemy) {
					target = world.field().enemy_goal();
				} else {
					target = world.field().friendly_goal();
				}

				Action::pivot(world, player, target);
			}
	};

	HighLevel::Ptr TestPivotFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestPivot(world));
		return p;
	}
}

