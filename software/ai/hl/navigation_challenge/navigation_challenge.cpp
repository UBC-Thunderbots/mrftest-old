#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	DoubleParam pos_dis_threshold("pos distance threshold", "NC", 0.05, 0, 1.0);
//	DoubleParam pos_vel_threshold("pos velocity threshold", "NC", 1.0, 0, 10.0);
//	DoubleParam ori_dis_threshold("ori distance threshold", "NC", 0.1, 0, 1.0);
//	DoubleParam ori_vel_threshold("ori velocity threshold", "NC", 0.03, 0, 1.0);

	const std::pair<Point, double> default_tasks[] = {
		std::make_pair(Point(2.7, 0.5), 0),
		std::make_pair(Point(2.8, 0), 0),
		std::make_pair(Point(2.7, -0.5), 0),
		std::make_pair(Point(-2.7, -0.5), 0),
		std::make_pair(Point(-2.7, 0), 0),
		std::make_pair(Point(-2.7, 0.5), 0)
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
			NCHL(World &world) : world(world), tasks(default_tasks, default_tasks + default_tasks_n), time_steps(0), done(0) {
			}

			NCHLFactory &factory() const {
				return factory_instance;
			}

			void tick() {
				FriendlyTeam &friendly = world.friendly_team();

				if(friendly.size() == 0)
					return;

				time_steps++;

				Player::Ptr runner = friendly.get(0);

				const Point diff_pos = runner->position() - tasks[done].first;

				if (diff_pos.len() < pos_dis_threshold) {
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

				runner->move(tasks[done].first, tasks[done].second, 0, AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::HIGH);
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

	HighLevel::Ptr NCHLFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new NCHL(world));
		return p;
	}
}

