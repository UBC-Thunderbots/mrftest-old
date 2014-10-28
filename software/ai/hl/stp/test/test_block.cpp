#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/action/block.h"
#include "ai/hl/stp/test/test.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestBlock final : public HighLevel {
		public:
			explicit TestBlock(World world) : world(world) {
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
				EnemyTeam enemy = world.enemy_team();
				if (!friendly.size() || !enemy.size()) {
					return;
				}

				Action::block_goal(world, friendly[0], enemy[0]);
			}
	};
}

HIGH_LEVEL_REGISTER(TestBlock)

