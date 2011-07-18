#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	DoubleParam pos_dis_threshold("pos distance threshold", "MB", 0.05, 0, 1.0);
	DoubleParam pos_vel_threshold("pos velocity threshold", "MB", 0.03, 0, 1.0);
	DoubleParam ori_dis_threshold("ori distance threshold", "MB", 0.1, 0, 1.0);
	DoubleParam ori_vel_threshold("ori velocity threshold", "MB", 0.03, 0, 1.0);

	const double PI = M_PI;

	const std::pair<Point, double> tasks_default[] = {
		std::make_pair(Point(1.2, 0), 0),
		std::make_pair(Point(1.5, 0), -PI / 2),
		std::make_pair(Point(1.2, 0.3), 0),
		std::make_pair(Point(1.2, -0.3), PI),
		std::make_pair(Point(1.2, 0), 0),
		std::make_pair(Point(1.2, -0.3), PI),
		std::make_pair(Point(1.2, 0), 0),
		std::make_pair(Point(0.5, 0), PI),
		std::make_pair(Point(2.5, 0), 0),
		std::make_pair(Point(0.5, 1.2), PI),
		std::make_pair(Point(1, -0.6), 0),
		std::make_pair(Point(2, 0.6), PI / 2),
		std::make_pair(Point(1, -0.6), -PI / 2),
		std::make_pair(Point(0.5, 0), 0),
		std::make_pair(Point(2.5, 0.6), -PI / 2)
	};

	const std::pair<Point, double> tasks_square[] = {
		std::make_pair(Point(2.0, 0.6), 0),
		std::make_pair(Point(2.0, -0.6), 0),
		std::make_pair(Point(0.5, -0.6), 0),
		std::make_pair(Point(0.5, 0.6), 0),
		std::make_pair(Point(2.0, 0.6), 0),
		std::make_pair(Point(2.0, -0.6), 0),
		std::make_pair(Point(0.5, -0.6), 0),
		std::make_pair(Point(0.5, 0.6), 0),
		std::make_pair(Point(2.0, 0.6), 0),
	};


	const int tasks_default_n = G_N_ELEMENTS(tasks_default);

	const int tasks_square_n = G_N_ELEMENTS(tasks_square);

	class MBHLFactory : public HighLevelFactory {
		public:
			MBHLFactory() : HighLevelFactory("Movement Benchmark") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	MBHLFactory factory_instance;

	class MBHL : public HighLevel {
		public:
			Gtk::VBox vbox;
			Gtk::Button button_normal;
			Gtk::Button button_square;
			Gtk::HScale rchoose;

			void start_normal() {
				done = 0;
				tasks.assign(tasks_default, tasks_default + tasks_default_n);
			}

			void start_square() {
				done = 0;
				tasks.assign(tasks_square, tasks_square + tasks_square_n);
			}

			MBHL(World &world) : world(world), time_steps(0) {
				done = 999;

				vbox.add(button_normal);
				vbox.add(button_square);
				button_normal.set_label("normal");
				button_square.set_label("square");

				button_normal.signal_clicked().connect(sigc::bind(&MBHL::start_normal, sigc::ref(*this)));
				button_square.signal_clicked().connect(sigc::bind(&MBHL::start_square, sigc::ref(*this)));

				rchoose.get_adjustment()->configure(0, 0, 10, 1, 10, 0);
				rchoose.set_digits(0);

				vbox.add(rchoose);
			}

			MBHLFactory &factory() const {
				return factory_instance;
			}

			void tick() {
				if (done > tasks.size()) {
					return;
				}

				FriendlyTeam &friendly = world.friendly_team();
				if (friendly.size() < 1) {
					// LOG_INFO("error: must have at exactly one robot on the field!");
					LOG_INFO("error: must have at least one robot on the field!");
					return;
				}
				time_steps++;

				unsigned int index = static_cast<unsigned int>(rchoose.get_value());

				Player::Ptr runner;

				for (std::size_t i = 0; i < friendly.size(); ++i) {
					if (friendly.get(i)->pattern() == index) {
						runner = friendly.get(i);
					}
				}
				if (!runner.is()) {
					return;
				}

				const Point diff_pos = runner->position() - tasks[done].first;
				const double diff_ori = angle_diff(runner->orientation(), tasks[done].second);

				if (diff_pos.len() < pos_dis_threshold && runner->velocity().len() < pos_vel_threshold && diff_ori < ori_dis_threshold && runner->avelocity() < ori_vel_threshold) {
					if (done == 0) {
						time_steps = 0;
					}
					++done;
				}

				if (done == tasks.size()) {
					LOG_INFO(Glib::ustring::compose("time steps taken: %1", time_steps));
					++done;
					return;
				}

				runner->move(tasks[done].first, tasks[done].second, 0, AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::HIGH);
			}

			Gtk::Widget *ui_controls() {
				return &vbox;
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

