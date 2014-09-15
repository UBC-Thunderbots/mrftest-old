#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/tactic/tdefend.h"
#include "ai/hl/stp/test/test.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestTDefend : public HighLevel {
		public:
			explicit TestTDefend(World world) : world(world) {
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
				if (friendly.size() < 4) {
					return;
				}

				auto goalie = Tactic::tgoalie(world, 2);
				goalie->set_player(friendly[0]);
				goalie->execute();

				auto defend1 = Tactic::tdefender1(world);
				defend1->set_player(friendly[1]);
				defend1->execute();

				auto defend2 = Tactic::tdefender2(world);
				defend2->set_player(friendly[2]);
				defend2->execute();

				auto defend3 = Tactic::tdefender3(world);
				defend3->set_player(friendly[3]);
				defend3->execute();

				// don't go into friendly defense area
				for (std::size_t i = 1 ; i < 4 ; i++){
					friendly[i].flags(0x0008);
				}
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_defense(world, ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(TestTDefend)

