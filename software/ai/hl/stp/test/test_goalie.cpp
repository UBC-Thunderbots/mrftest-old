#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/test/test.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestLoneGoalie final : public HighLevel {
		public:
			explicit TestLoneGoalie(World world) : world(world) {
			}

		private:
			World world;

			HighLevelFactory &factory() const override;

			Gtk::Widget *ui_controls() override {
				return nullptr;
			}

			void tick() override {
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

