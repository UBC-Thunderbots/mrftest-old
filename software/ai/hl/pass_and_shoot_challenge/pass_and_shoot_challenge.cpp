#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include "ai/flags.h"

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	const unsigned int min_team_size = 4;

	Point bot0_initial(-2.75,-1.2);
	Point bot1_initial(-2.00,1.2);
	Point bot2_initial(-1.00,-1.2);
	Point bot3_initial(0.00,1.2);
	Angle robot0_orientation = (bot1_initial - bot0_initial).orientation();
	Angle robot1_orientation = (bot0_initial - bot1_initial).orientation();
	Angle robot2_orientation = (bot1_initial - bot2_initial).orientation();
	Angle robot3_orientation = (bot2_initial - bot3_initial).orientation();


};


class PASCHL : public HighLevel {
		public:
			PASCHL(World &world) : world(world) {
				robot_positions.push_back(std::make_pair(bot0_initial, robot0_orientation));
				robot_positions.push_back(std::make_pair(bot1_initial,robot1_orientation));
				robot_positions.push_back(std::make_pair(bot2_initial,robot2_orientation));
				robot_positions.push_back(std::make_pair(bot3_initial,robot3_orientation));

			}

			HighLevelFactory &factory() const;

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();

				if (friendly.size() < min_team_size) {
					return;
				}
				for(unsigned int i = 0 ;i < min_team_size; i++) {
					friendly.get(i)->move(robot_positions[i].first, robot_positions[i].second, Point());
				}


			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

		private:
			World &world;
			std::vector<std::pair<Point, Angle>> robot_positions;

};


HIGH_LEVEL_REGISTER(PASCHL)

