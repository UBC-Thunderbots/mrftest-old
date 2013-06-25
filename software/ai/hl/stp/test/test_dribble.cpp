/*
 * test_dribble.cpp
 *
 *  Created on: 2013-06-25
 *      Author: somik
 */
#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/test/test.h"
#include "ai/hl/stp/action/chip.h"
#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/predicates.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;


namespace {
	class TestDribble: public HighLevel {
		public:
			TestDribble(World world) : world(world) {
			}

		private:
			World world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				tick_eval(world);
				bool north = false;
				bool south = false;
				FriendlyTeam friendly = world.friendly_team();
				std::vector<Player> players;

				if (friendly.size() == 0) {
					return;
				}

				//if(AI::HL::STP::Predicates::our_ball(world)) {
			//		Action::dribble(world, friendly.get(0), Point(0, 1.5));
			//	} else {
					Action::intercept(friendly.get(0), world.ball().position());
			//	}
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_shoot(world, ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(TestDribble)




