#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/test/test.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestRepel : public HighLevel {
		public:
			explicit TestRepel(World world) : world(world) {
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

				Action::repel(world, friendly[0]);
			}
	};
}

HIGH_LEVEL_REGISTER(TestRepel)

