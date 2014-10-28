#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/test/test.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/coordinate.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class TestChip final : public HighLevel {
		public:
			explicit TestChip(World world) : world(world) {
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
				// this wont work (not implemented) in the ode simulator
				// use a real robot or grsim instead 
				Action::chip_target(world, friendly[0], world.field().enemy_goal());
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) override {
				draw_shoot(world, ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(TestChip)

