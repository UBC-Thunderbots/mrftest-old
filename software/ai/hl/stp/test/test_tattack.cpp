#include "ai/hl/hl.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/tactic/tdefend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/test/test.h"

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestTAttack : public HighLevel {
		public:
			TestTAttack(World world) : world(world) {
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

				Action::shoot_goal(world, friendly.get(0));

				auto attack1 = Tactic::tdefend_line(world, Coordinate(world, Point(-1.1, 0.25), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, Point(-0.7, 0.5), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 1.5);
				attack1->set_player(friendly.get(1));
				attack1->execute();

				auto attack2 = Tactic::tdefend_line(world, Coordinate(world, Point(-1.1, 0.35), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, Point(-0.7, -0.5), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 1.5);
				attack2->set_player(friendly.get(2));
				attack2->execute();
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_offense(world, ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(TestTAttack)

