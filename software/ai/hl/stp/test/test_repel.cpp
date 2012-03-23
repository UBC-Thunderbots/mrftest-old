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
			TestRepel(World &world) : world(world) {
			}

		private:
			World &world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				tick_eval(world);

				FriendlyTeam &friendly = world.friendly_team();
				if (friendly.size() == 0) {
					return;
				}

				Action::repel(world, friendly.get(0));
			}
	};
}

HIGH_LEVEL_REGISTER(TestRepel)

