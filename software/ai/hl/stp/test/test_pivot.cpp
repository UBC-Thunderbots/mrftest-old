#include "ai/hl/hl.h"
#include "ai/hl/stp/action/pivot.h"
#include "util/dprint.h"
#include "geom/angle.h"

#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	int time;

	class TestPivotFactory : public HighLevelFactory {
		public:
			TestPivotFactory() : HighLevelFactory("Test STP Pivot") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestPivotFactory factory_instance;

	class TestPivot : public HighLevel {
		public:
			TestPivot(World &world) : world(world), target_enemy(false) {
				time = 0;
			}

		private:
			World &world;
			bool target_enemy;

			TestPivotFactory &factory() const {
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

				Point target;

				if (target_enemy) {
					target = world.field().enemy_goal();
				} else {
					target = world.field().friendly_goal();
				}

				const Angle diff_ori = player->orientation().angle_diff((target - player->position()).orientation());
				if (diff_ori < Angle::of_radians(0.1)) {
					// angle completed, switch goals.
					target_enemy = !target_enemy;
					LOG_INFO(Glib::ustring::compose("time steps taken: %1", time));
					time = 0;
				}
				time++;

				Action::pivot(player, target);
			}
	};

	HighLevel::Ptr TestPivotFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestPivot(world));
		return p;
	}
}

