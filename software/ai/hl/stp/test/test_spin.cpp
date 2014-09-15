#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/action/move_spin.h"
#include "ai/hl/stp/test/test.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestSpin : public HighLevel {
		public:
			explicit TestSpin(World world) : world(world) {
			}

		private:
			World world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return nullptr;
			}

			void tick() {
				tick_eval(world);

				FriendlyTeam friendly = world.friendly_team();
				if (!friendly.size()) {
					return;
				}
				Point dirToBall = (world.ball().position() - friendly[0].position()).norm();
				Action::move_spin(friendly[0], world.ball().position() + Robot::MAX_RADIUS * dirToBall);
			}
	};
}

HIGH_LEVEL_REGISTER(TestSpin)

