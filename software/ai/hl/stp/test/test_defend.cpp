#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/defend.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/test/test.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestDefend : public HighLevel {
		public:
			TestDefend(World world) : world(world) {
			}

		private:
			World world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				tick_eval(world);

				FriendlyTeam friendly = world.friendly_team();
				if (friendly.size() < 3) {
					return;
				}

				auto waypoints = Evaluation::evaluate_defense();
				Action::goalie_move(world, friendly[0], waypoints[0]);
				Action::defender_move(world, friendly[1], waypoints[1]);
				Action::defender_move(world, friendly[2], waypoints[2]);
				
				// don't go into friendly defense area
				for (std::size_t i = 1 ; i < 3 ; i++){
					friendly[i].flags(0x0008);
				}
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_defense(world, ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(TestDefend)

