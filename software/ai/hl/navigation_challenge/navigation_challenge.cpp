#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include "ai/flags.h"

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	DoubleParam pos_dis_threshold(u8"pos distance threshold", u8"NC", 0.2, 0, 1.0);
	DoubleParam x_diff(u8"distance in the x direction to set the navigation points", u8"NC", 0.3, 0, 1.0);
	DoubleParam y_diff(u8"distance in the y direction to set the navigation points", u8"NC", 0.5, 0, 1.0);

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
			NCHL(World world) : world(world), tasks(default_tasks, default_tasks + default_tasks_n), time_steps(0) {
				std::vector<std::size_t> done;
				obstacleIndex = 0;
			}

			HighLevelFactory &factory() const;

			void tick() {
				FriendlyTeam friendly = world.friendly_team();

				if (!friendly.size()) {
					return;
				}

				if (done.size() < 3 && friendly.size() != done.size()) {
					done.clear();
					std::size_t taskIndex = 0;
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
					Robot leftmostRobot;
					Point rightmost(0, 0);
					Robot rightmostRobot;
					for (Robot i : world.enemy_team()) {
						Point location = i.position();
						if (location.x < leftmost.x) {
							leftmost = location;
							leftmostRobot = i;
						} else if (location.x > rightmost.x) {
							rightmost = location;
							rightmostRobot = i;
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
					for (Robot i : world.enemy_team()) {
						if (i == leftmostRobot || i == rightmostRobot) {
							i.avoid_distance(AI::Flags::AvoidDistance::MEDIUM);
						} else {
							i.avoid_distance(AI::Flags::AvoidDistance::LONG);
						}
					}
				}

				time_steps++;

				// Set up to three players
				for (std::size_t robotIndex = 0; robotIndex < friendly.size() && robotIndex < 3; ++robotIndex) {
					Player runner = friendly[robotIndex];

					const Point diff_pos = runner.position() - tasks[done[robotIndex]].first;

					if (diff_pos.len() < pos_dis_threshold) {
						if (done[robotIndex] == 0) {
							time_steps = 0;
						}
						++done[robotIndex];
					}

					if (done[robotIndex] >= tasks.size()) {
						LOG_INFO(Glib::ustring::compose(u8"time steps taken: %1", time_steps));
						time_steps = 0;
						done[robotIndex] = 0;
						return;
					}
					Angle dest_ori = (tasks[done[robotIndex]].first - runner.position()).orientation();

					runner.flags(0);
					runner.type(AI::Flags::MoveType::NORMAL);
					runner.prio(AI::Flags::MovePrio::HIGH);
					runner.move(tasks[done[robotIndex]].first, dest_ori, Point());
				}

				// Set moving obstacles
				if (friendly.size() >= 4) {
					Player runner = friendly[3];
					Point des;
					if (obstacleIndex == 0) {
						des = Point(0, 1);
					} else {
						des = Point(0, -1);
					}
					const Point diff_pos = runner.position() - des;
					if (diff_pos.len() < pos_dis_threshold) {
						obstacleIndex++;
						if (obstacleIndex > 1) {
							obstacleIndex = 0;
						}
					}

					runner.flags(0);
					runner.type(AI::Flags::MoveType::NORMAL);
					runner.prio(AI::Flags::MovePrio::HIGH);
					runner.move(des, Angle::zero(), Point());
				}
			}

			Gtk::Widget *ui_controls() {
				return nullptr;
			}

		private:
			World world;
			std::vector<std::pair<Point, double>> tasks;
			int time_steps;
			std::vector<std::size_t> done;
			int obstacleIndex;
	};
}

HIGH_LEVEL_REGISTER(NCHL)

