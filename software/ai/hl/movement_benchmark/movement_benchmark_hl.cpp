#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	const double pos_dis_threshold = 0.2;
	const double pos_vel_threshold = 0.2;
	const double ori_dis_threshold = 0.2;
	const double ori_vel_threshold = 0.2;

	const double PI = M_PI;
	
	const std::pair<Point, double> default_tasks[] = {
		std::make_pair(Point(1.2, 0), 0),
		std::make_pair(Point(0.5, 0), PI),
		std::make_pair(Point(2.5, 0), 0),
		std::make_pair(Point(0.5, 1.2), PI),
		std::make_pair(Point(1, -0.6), 0)
	};
	
	const int default_tasks_n = G_N_ELEMENTS(default_tasks);
	
	class MBHLFactory : public HighLevelFactory {
		public:
			MBHLFactory() : HighLevelFactory("Movement Benchmark") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	MBHLFactory factory_instance;

	class MBHL : public HighLevel {
		public:
			MBHL(World &world) : world(world), tasks(default_tasks, default_tasks + default_tasks_n) {
			}

			MBHLFactory &factory() const {
				return factory_instance;
			}

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();
				if (friendly.size() != 0) {
					LOG_INFO("error: must have at exactly one robot on the field!");
					return;
				}
				time_steps++;

				Player::Ptr runner = friendly.get(0);

				const Point diff_pos = runner->position() - tasks[done].first;
				const double diff_ori = angle_mod(runner->orientation() - tasks[done].second);

				if (diff_pos.len() < pos_dis_threshold && runner->velocity().len() < pos_vel_threshold && fabs(diff_ori) < ori_dis_threshold && runner->avelocity() < ori_vel_threshold) {
					if (done == 0) {
						time_steps = 0;
					}
					++done;
				}

				if (done >= tasks.size()) {
					LOG_INFO(Glib::ustring::compose("time steps taken: %1", time_steps));
					time_steps = 0;
					done = 0;
					return;
				}

				runner->move(tasks[done].first, tasks[done].second, 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
			}

			Gtk::Widget *ui_controls() {
				return 0;
			}
			
		private:
			World &world;
			std::vector<std::pair<Point, double> > tasks;
			int time_steps;
			std::size_t done;
	};

	HighLevel::Ptr MBHLFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new MBHL(world));
		return p;
	}
}

