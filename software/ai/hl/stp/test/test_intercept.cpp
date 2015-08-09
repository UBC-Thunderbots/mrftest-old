#include "ai/hl/hl.h"
#include "ai/flags.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/stp/action/intercept.h"

using namespace AI::HL;
using namespace AI::HL::W;
using namespace AI::HL::STP;

namespace {
	class TestIntercept final : public HighLevel {
		public:
			explicit TestIntercept(World world) : world(world) {
			}

		private:
			World world;

			HighLevelFactory &factory() const override;

			Gtk::Widget *ui_controls() override {
				return nullptr;
			}

			void tick() override {
				FriendlyTeam friendly = world.friendly_team();
				if (!friendly.size()) {
					return;
				}

				Player player = friendly[0];
#warning This does not work with movement primitives. It used to call autokick.
				Action::intercept(player, world.field().enemy_goal());
			}
	};
}

HIGH_LEVEL_REGISTER(TestIntercept)

