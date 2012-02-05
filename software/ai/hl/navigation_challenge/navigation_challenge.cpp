#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include "ai/flags.h"

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

	class NCHL : public HighLevel {
		public:
			NCHL(World &world) : world(world), tasks(default_tasks, default_tasks + default_tasks_n), time_steps(0) {
				std::vector<std::size_t> done;
				obstacleIndex = 0;
			}

			HighLevelFactory &factory() const;

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();

				if (friendly.size() == 0) {
					return;
				}

				if (done.size() < 3 && friendly.size() != done.size()) {
					done.clear();
					int taskIndex = 0;
					for (unsigned int i = 0; i < friendly.size() && i < 3; ++i) {
						done.push_back(taskIndex);
						if (taskIndex == 0) {
							taskIndex = 3;
						} else {
							taskIndex = 0;
						}
					}
				}

				if (world.enemy_team().size() >= 2) {
					Point leftmost(0, 0);
					unsigned int leftmostIndex = 0;
					Point rightmost(0, 0);
					unsigned int rightmostIndex = 0;
					for (unsigned int i = 0; i < world.enemy_team().size(); ++i) {
						Point location = world.enemy_team().get(i)->position();
						if (location.x < leftmost.x) {
							leftmost = location;
							leftmostIndex = i;
						} else if (location.x > rightmost.x) {
							rightmost = location;
							rightmostIndex = i;
						}
					}

					// set points relative to enemy robots
					tasks[0] = std::make_pair(Point(leftmost.x, leftmost.y + y_diff), 0);
					tasks[1] = std::make_pair(Point(leftmost.x - x_diff, leftmost.y - 0.1), 0);
					tasks[2] = std::make_pair(Point(leftmost.x, leftmost.y - y_diff), 0);
					tasks[3] = std::make_pair(Point(rightmost.x, rightmost.y - y_diff), 0);
					tasks[4] = std::make_pair(Point(rightmost.x + x_diff, rightmost.y + 0.1), 0);
					tasks[5] = std::make_pair(Point(rightmost.x, rightmost.y + y_diff), 0);

					// set avoidance distance based on whether the robot is moving
					for (unsigned int i = 0; i < world.enemy_team().size(); ++i) {
						if (i == leftmostIndex || i == rightmostIndex) {
							world.enemy_team().get(i)->avoid_distance(AI::Flags::AvoidDistance::MEDIUM);
						} else {
							world.enemy_team().get(i)->avoid_distance(AI::Flags::AvoidDistance::LONG);
						}
					}
				}

				time_steps++;

				// Set up to three players
				for (unsigned int robotIndex = 0; robotIndex < friendly.size() && robotIndex < 3; ++robotIndex) {
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
					Angle dest_ori = (tasks[done[robotIndex]].first - runner->position()).orientation();

					runner->flags(0);
					runner->type(AI::Flags::MoveType::NORMAL);
					runner->prio(AI::Flags::MovePrio::HIGH);
					runner->move(tasks[done[robotIndex]].first, dest_ori, Point());
				}

				// Set moving obstacles
				if (friendly.size() >= 4) {
					Player::Ptr runner = friendly.get(3);
					Point des;
					if (obstacleIndex == 0) {
						des = Point(0, 1);
					} else {
						des = Point(0, -1);
					}
					const Point diff_pos = runner->position() - des;
					if (diff_pos.len() < pos_dis_threshold) {
						obstacleIndex++;
						if (obstacleIndex > 1) {
							obstacleIndex = 0;
						}
					}

					runner->flags(0);
					runner->type(AI::Flags::MoveType::NORMAL);
					runner->prio(AI::Flags::MovePrio::HIGH);
					runner->move(des, Angle::ZERO, Point());
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
			int obstacleIndex;
	};
}

HIGH_LEVEL_REGISTER(NCHL)

