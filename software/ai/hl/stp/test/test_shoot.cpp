#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/test/test.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestShoot final : public HighLevel {
		public:
			explicit TestShoot(World world) : world(world) {
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
				

				Action::shoot_goal(world, friendly[0]);
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) override {
				draw_shoot(world, ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(TestShoot)

