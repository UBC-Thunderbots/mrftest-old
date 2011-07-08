#include "ai/hl/hl.h"
#include "ai/hl/stp/action/pivot.h"
#include "util/dprint.h"
#include "geom/angle.h"
#include "ai/hl/stp/stp.h"

#include <sstream>
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
				tick_eval(world);

				FriendlyTeam &friendly = world.friendly_team();
				if (friendly.size() == 0) {
					return;
				}

				Player::Ptr player = friendly.get(0);

				Point target;

				/*if (target_enemy) {
					target = world.field().e;
				} else {
					target = world.field().friendly_goal();
				}

				const double diff_ori = angle_diff(player->orientation(), (target - player->position()).orientation());
				std::stringstream ss;
				ss << diff_ori;
				LOG_INFO( ss.str() );
				if (diff_ori < 0.1) {
					// angle completed, switch goals.
					target_enemy = !target_enemy;
					LOG_INFO(Glib::ustring::compose("time steps taken: %1", time));
					time = 0;
				}
				time++;*/

				Action::pivot(world, player, Point(0.0,0.0));//aim at center of field
			}
	};

	HighLevel::Ptr TestPivotFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestPivot(world));
		return p;
	}
}

