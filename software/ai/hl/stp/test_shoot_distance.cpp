#include "ai/hl/hl.h"
#include "ai/hl/stp/action/shoot.h"
#include "util/dprint.h"

#include <cassert>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;

namespace {
	class TestShootDistanceFactory : public HighLevelFactory {
		public:
			TestShootDistanceFactory() : HighLevelFactory("Tune STP Shoot Distance") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	TestShootDistanceFactory factory_instance;

	class TestShootDistance : public HighLevel {
		public:
			TestShootDistance(World &world) : world(world) {
			}

		private:
			World &world;

			TestShootDistanceFactory &factory() const {
				return factory_instance;
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {
#warning Finish Coding Shoot Distance Tuning Test
				//Assert that there are only 1 team members on the team
				FriendlyTeam &friendly = world.friendly_team();
				if (friendly.size() > 1 || friendly.size() <= 0 ) {
					return;
				}

				// if high or low value of test is good enough then great lets grab it and print out tunable params



				// If ball is still and away from corner
					// If "kicked" is true calculate how close the distance test was
						//update various parameters
					// set "kicked" to false

				// if kicked don't do anything

				// If not kicked
						// If ball not still and in a corner
							// chase after the ball and move the ball to the closest/fixxed corner
						// else do kicking process 
							//kicked = a successful kick happened




				Action::shoot_goal(world, friendly.get(0));
			}
	};

	HighLevel::Ptr TestShootDistanceFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new TestShootDistance(world));
		return p;
	}
}

