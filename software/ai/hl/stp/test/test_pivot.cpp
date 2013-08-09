#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/test/test.h"
#include "geom/angle.h"
#include "util/dprint.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {

	class TestPivot : public HighLevel {
		public:
			TestPivot(World world) : world(world), target_enemy(false) {
				time = 0;
			}

		private:
			World world;
			bool target_enemy;
			int time;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return nullptr;
			}

			void tick() {
				FriendlyTeam friendly = world.friendly_team();
				if (!friendly.size()) {
					return;
				}

				Player player = friendly[0];

				Point target;

				if (target_enemy) {
					target = world.field().enemy_goal();
				} else {
					target = world.field().friendly_goal();
				}

				const Angle diff_ori = player.orientation().angle_diff((target - player.position()).orientation());
				if (diff_ori < Angle::of_radians(0.1)) {
					// angle completed, switch goals.
					target_enemy = !target_enemy;
					LOG_INFO(Glib::ustring::compose(u8"time steps taken: %1", time));
					time = 0;
				}
				time++;

				Action::pivot(world, player, target, 0.15);
			}
	};
}

HIGH_LEVEL_REGISTER(TestPivot)

