#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	DoubleParam pos_dis_threshold("pos distance threshold", "NC", 0.2, 0, 1.0);
	DoubleParam x_diff("distance in the x direction to set the navigation points", "NC", 0.3, 0, 1.0);
	DoubleParam y_diff("distance in the y direction to set the navigation points", "NC", 0.5, 0, 1.0);

	const std::pair<Point, double> default_tasks[] = {
		std::make_pair(Point(-2.7, 0.5), 0),
		std::make_pair(Point(-3.0, 0), 0),
		std::make_pair(Point(-2.7, -0.5), 0),
		std::make_pair(Point(2.7, -0.5), 0),
		std::make_pair(Point(3.0, 0), 0),
		std::make_pair(Point(2.7, 0.5), 0)
	};

	const int default_tasks_n = G_N_ELEMENTS(default_tasks);

	class NCHLFactory : public HighLevelFactory {
		public:
			NCHLFactory() : HighLevelFactory("Navigation Challenge") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	NCHLFactory factory_instance;

	class NCHL : public HighLevel {
		public:
			NCHL(World &world) : world(world), tasks(default_tasks, default_tasks + default_tasks_n), time_steps(0) {
				std::vector<std::size_t> done;
			}

			NCHLFactory &factory() const {
				return factory_instance;
			}

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();

				if (friendly.size() == 0)
					return;
					
				if (done.size() != friendly.size()) {
					done.clear();
					int taskIndex = 0;
					for (unsigned int i = 0; i < friendly.size(); ++i) {
						done.push_back(taskIndex);
						if (taskIndex == 0) taskIndex = 3;
						else taskIndex = 0;
					}
				}

				if (world.enemy_team().size() >= 2) {
					Point leftmost(0, 0);
					Point rightmost(0, 0);
					for (unsigned int i = 0; i < world.enemy_team().size(); ++i) {
						Point location = world.enemy_team().get(i)->position();
						if (location.x < leftmost.x) {
							leftmost = location;
						} else if (location.x > rightmost.x) {
							rightmost = location;
						}
					}

					// set points relative to enemy robots
					tasks[0] = std::make_pair(Point(leftmost.x, leftmost.y + y_diff), 0);
					tasks[1] = std::make_pair(Point(leftmost.x - x_diff, leftmost.y), 0);
					tasks[2] = std::make_pair(Point(leftmost.x, leftmost.y - y_diff), 0);
					tasks[3] = std::make_pair(Point(rightmost.x, rightmost.y - y_diff), 0);
					tasks[4] = std::make_pair(Point(rightmost.x + x_diff, rightmost.y), 0);
					tasks[5] = std::make_pair(Point(rightmost.x, rightmost.y + y_diff), 0);
				}

				time_steps++;
				
				for (unsigned int robotIndex = 0; robotIndex < friendly.size(); ++robotIndex) {
					Player::Ptr runner = friendly.get(robotIndex);

					const Point diff_pos = runner->position() - tasks[done[robotIndex]].first;

					if (diff_pos.len() < pos_dis_threshold) {
						if (done[robotIndex] == 0) {
							time_steps = 0;
						}
						++done[robotIndex];
					}

					if (done[robotIndex] >= tasks.size()) {
						LOG_INFO(Glib::ustring::compose("time steps taken: %1", time_steps));
						time_steps = 0;
						done[robotIndex] = 0;
						return;
					}
					double dest_ori = (tasks[done[robotIndex]].first - runner->position()).orientation();
					runner->move(tasks[done[robotIndex]].first, dest_ori, 0, AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::HIGH);
				}
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}

		private:
			World &world;
			std::vector<std::pair<Point, double> > tasks;
			int time_steps;
			std::vector<std::size_t> done;
	};

	HighLevel::Ptr NCHLFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new NCHL(world));
		return p;
	}
}

