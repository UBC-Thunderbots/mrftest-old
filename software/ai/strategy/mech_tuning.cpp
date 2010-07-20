#include "ai/strategy/movement_benchmark.h"
#include "robot_controller/tunable_controller.h"
#include "geom/angle.h"

#include <iomanip>
#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>
#include <gtkmm.h>

namespace {

	const double PI = M_PI;

	class MechTuning;

	/** Enables parameter tuning by hand.
	 */
	class MechTuningUI : public Gtk::Window {
		public:
			MechTuningUI(MechTuning* h) : ht(h) {
				set_title("Parameters");
				//vbox.add(button);
				//button.signal_clicked().connect(sigc::mem_fun(this, &MechTuningUI::run));
				//button.set_label("Run!");
				//add(vbox);
				add(table);
			}

			~MechTuningUI() {
				for (size_t i = 0; i < entries.size(); ++i) {
					table.remove(*entries[i]);
					table.remove(*labels[i]);
					delete entries[i];
					delete labels[i];
				}
			}

			// void run();

			/// Resets the parameters on the ui.
			void reset(TunableController* tc) {
				for (size_t i = 0; i < entries.size(); ++i) {
					table.remove(*entries[i]);
					table.remove(*labels[i]);
					delete entries[i];
					delete labels[i];
				}
				if (tc == NULL) {
					entries.clear();
					labels.clear();
					hide_all();
					return;
				}
				size_t P = tc->get_params().size();
				const std::vector<std::string>& names = tc->get_params_name();
				const std::vector<double>& vals = tc->get_params();
				labels.resize(P);
				entries.resize(P);
				table.resize(P, 2);
				for (size_t i = 0; i < P; ++i) {
					labels[i] = new Gtk::Label(names[i]);
					entries[i] = new Gtk::Entry();
					table.attach(*labels[i], 0, 1, i, i+1);
					table.attach(*entries[i], 1, 2, i, i+1);
					Glib::ustring str = Glib::ustring::format(std::fixed, std::setprecision(3), vals[i]);
					entries[i]->set_text(str);
				}
				show_all();
			}

			const std::vector<double> read_params() const {
				std::vector<double> ret(entries.size(), 0);
				for (size_t i = 0; i < entries.size(); ++i) {
					Glib::ustring str = entries[i]->get_text();
					ret[i] = atof(str.c_str());
				}
				return ret;
			}

		protected:
			bool on_delete_event(GdkEventAny *) {
				Gtk::Main::quit();
				return true;
			}

		private:
			int params;
			Gtk::Table table;
			std::vector<Gtk::Label*> labels;
			std::vector<Gtk::Entry*> entries;
			MechTuning* ht;
	};

	class MechTuning : public MovementBenchmark {
		public:
			MechTuning(RefPtr<World>);
			~MechTuning();
			Gtk::Widget *get_ui_controls();
			StrategyFactory &get_factory();
			void tick();
			void run_turn();
			void run_bench();
			void run_err();
			void reset();
		private:
			MechTuningUI ui;
			TunableController* tc;
			Gtk::Button run_bench_button;
			Gtk::Button run_turn_button;
			Gtk::Button run_err_button;
			Gtk::Button reset_button;
			Gtk::HScale x_scale;
			Gtk::HScale y_scale;
			Gtk::HScale a_scale;
			Gtk::VBox vbox;
			time_t start_phase;
			int phase;
	};

	MechTuning::MechTuning(RefPtr<World> world) : MovementBenchmark(world), ui(this), tc(NULL), run_bench_button("Run Benchmark"), run_turn_button("Turning for 5 mins"), run_err_button("Run error for x/y/angle"), reset_button("reset"), x_scale(-2.0, 2.0, 0.01), y_scale(-2.0, 2.0, 0.01), a_scale(-4.0, 4.0, 0.01) {
		run_bench_button.signal_clicked().connect(sigc::mem_fun(this,&MechTuning::run_bench));
		run_turn_button.signal_clicked().connect(sigc::mem_fun(this,&MechTuning::run_turn));
		run_err_button.signal_clicked().connect(sigc::mem_fun(this,&MechTuning::run_err));
		reset_button.signal_clicked().connect(sigc::mem_fun(this,&MechTuning::reset));
		done = tasks.size();
		phase = 0;
		time_steps = 0;
		vbox.add(run_bench_button);
		vbox.add(run_turn_button);
		vbox.add(run_err_button);
		vbox.add(reset_button);
		vbox.add(x_scale);
		vbox.add(y_scale);
		vbox.add(a_scale);

		x_scale.set_value(0);
		y_scale.set_value(0);
		a_scale.set_value(0);
	}

	MechTuning::~MechTuning() {
	}

	Gtk::Widget *MechTuning::get_ui_controls() {
		return &vbox;
	}

	void MechTuning::reset() {
		tc = TunableController::get_instance();
		time_steps = 0;
		phase = 3;
		done = tasks.size();
		ui.reset(tc);
		x_scale.set_value(0);
		y_scale.set_value(0);
		a_scale.set_value(0);
	}

	void MechTuning::run_bench() {
		if (tc) {
			const std::vector<double>& params = ui.read_params();
			tc->set_params(params);
		}
		done = 0;
		time_steps = 0;
		phase = 2;
	}

	void MechTuning::run_turn() {
		if (tc) {
			const std::vector<double>& params = ui.read_params();
			tc->set_params(params);
		}
		done = 0;
		time_steps = 0;
		phase = 0;
	}

	void MechTuning::run_err() {
		if (tc) {
			const std::vector<double>& params = ui.read_params();
			tc->set_params(params);
		}
		done = 0;
		time_steps = 0;
		phase = 1;
	}

	void MechTuning::tick() {
		const FriendlyTeam &friendly(the_world->friendly);

		if (friendly.size() != 1) {
			std::cerr << "error: must have only 1 robot in the team!" << std::endl;
			return;
		}

		if (tc != TunableController::get_instance()) {
			reset();
		}

		if (done == tasks.size()) {
			phase++;
			done = 0;
		}

		const RefPtr<Player> the_player = friendly.get_player(0);

		switch(phase) {
			case 0:
				{
					if (done == 0) {
						time_steps = 0;
						time(&start_phase);
						done++;
					}
					time_t end_phase;
					time(&end_phase);
					double diff = difftime(end_phase, start_phase);
					if(diff >= 5 * 60) {
						done = tasks.size();
						return;
					}
					prev_ori = the_player->orientation();
					prev_pos = the_player->position();
					if (fmod(diff, 6) >= 3) {
						the_player->move(prev_pos, prev_ori + 3 * PI / 2);
					} else {
						the_player->move(prev_pos, prev_ori - 3 * PI / 2);
					}
				}
				break;
			case 1:
				{
					const double ex = x_scale.get_value();
					const double ey = y_scale.get_value();
					const double ea = a_scale.get_value();
					//Point next_pos = the_player->position() + Point(ex, ey);
					//double next_ori = the_player->orientation() + ea;
					//the_player->move(next_pos, next_ori);
					the_player->move(Point(ex, ey), ea);
				}
				break;
			case 2:
				MovementBenchmark::tick();
				break;
		}
	}

	class MechTuningFactory : public StrategyFactory {
		public:
			MechTuningFactory();
			RefPtr<Strategy2> create_strategy(RefPtr<World> world);
	};

	MechTuningFactory::MechTuningFactory() : StrategyFactory("Mech Tuning Benchmark") {
	}

	RefPtr<Strategy2> MechTuningFactory::create_strategy(RefPtr<World> world) {
		RefPtr<Strategy2> s(new MechTuning(world));
		return s;
	}

	MechTuningFactory factory;

	StrategyFactory &MechTuning::get_factory() {
		return factory;
	}

}

