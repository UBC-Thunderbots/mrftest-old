#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/test/test.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestLoneGoalie : public HighLevel {
		public:
			explicit TestLoneGoalie(World world) : world(world) {
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

				Action::lone_goalie(world, friendly[0]);
			}
	};
}

HIGH_LEVEL_REGISTER(TestLoneGoalie)

