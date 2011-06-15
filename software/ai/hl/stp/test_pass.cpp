#include "ai/hl/hl.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/move.h"
#include "util/dprint.h"

#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	IntParam pass_target("passing target points", "STP/test_pass", 0, 0, 14);

	// make better targets

	const Point default_targets[] = {
		Point(1.2, 0),
		Point(1.5, 0),
		Point(1.2, 0.3),
		Point(1.2, -0.3),
		Point(1.2, 0),
		Point(1.2, -0.3),
		Point(1.2, 0),
		Point(0.5, 0),
		Point(2.5, 0),
		Point(0.5, 1.2),
		Point(1, -0.6),
		Point(2, 0.6),
		Point(1, -0.6),
		Point(0.5, 0),
		Point(2.5, 0.6)
	};

	const int default_targets_n = G_N_ELEMENTS(default_targets);

	class TestPassFactory : public HighLevelFactory {
		public:
			TestPassFactory() : HighLevelFactory("Test STP Pass") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestPassFactory factory_instance;

	class TestPass : public HighLevel {
		public:
			TestPass(World &world) : world(world), targets(default_targets, default_targets + default_targets_n) {
			}

		private:
			World &world;

			std::vector<Point> targets;

			TestPassFactory &factory() const {
				return factory_instance;
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
				std::vector<AI::HL::W::Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());
				if (players.size() != 2) {
					return;
				}

				std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));

				// passer grabs ball
				if (!players[0]->has_ball()) {
					Action::chase(world, players[0], targets[pass_target]);
				}

				// passee move to target
				Action::move(players[1], (world.ball().position() - players[1]->position()).orientation(), targets[pass_target]);
				// passer shoots
				bool kicked = false;

				if (players[0]->has_ball()) {
					if (Action::shoot_target(world, players[0], targets[pass_target], true)) { 
						kicked = true; 
					}
				}

				// passee grabs ball
				if (kicked) {
					Action::chase(world, players[1]);
				}
			}
	};

	HighLevel::Ptr TestPassFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestPass(world));
		return p;
	}
}

