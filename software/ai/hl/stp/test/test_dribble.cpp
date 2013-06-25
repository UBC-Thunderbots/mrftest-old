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
#include "ai/hl/stp/tactic/intercept.h"

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

//				Point::Point NW(-5.5,3.75);
//				Point::Point SW(-5.5,-3.75);
//				Point NE(-0.5, 3,75);
//				Point SW(-0.5, -3.75);

				if (friendly.size() == 0) {
					return;
				}

				for (std::size_t i = 0; i < friendly.size(); ++i) {
					players.push_back(friendly.get(i));
				}

				if (world.friendly_team().size() > 0) {
					auto baller = AI::HL::STP::Tactic::intercept(world, world.ball().position());
					baller->set_player(players[0]);
					baller->execute();
				}
//				Action::chip_target(world, friendly.get(0), world.field().enemy_goal());
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_shoot(world, ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(TestDribble)





