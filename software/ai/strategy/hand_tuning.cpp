#include "ai/strategy/movement_benchmark.h"
#include "robot_controller/tunable_controller.h"

#include <iomanip>
#include <iostream>
#include <vector>
#include <cmath>
#include <gtkmm.h>

namespace {

	class hand_tuning;

	/** Enables parameter tuning by hand.
	 */
	class hand_tuning_ui : public Gtk::Window {
		public:
			hand_tuning_ui(hand_tuning* h) : ht(h) {
				set_title("Parameters");
				//vbox.add(button);
				//button.signal_clicked().connect(sigc::mem_fun(this, &hand_tuning_ui::run));
				//button.set_label("Run!");
				//add(vbox);
				add(table);
			}

			~hand_tuning_ui() {
				for (size_t i = 0; i < entries.size(); ++i) {
					table.remove(*entries[i]);
					table.remove(*labels[i]);
					delete entries[i];
					delete labels[i];
				}
			}

			// void run();

			/// Resets the parameters on the ui.
			void reset(tunable_controller* tc) {
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
			// Gtk::VBox vbox;
			// Gtk::Button button;
			std::vector<Gtk::Label*> labels;
			std::vector<Gtk::Entry*> entries;
			hand_tuning* ht;
	};

	class hand_tuning : public movement_benchmark {
		public:
			hand_tuning(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			~hand_tuning();
			strategy_factory &get_factory();
			void tick();
			void run();
			void reset();
		private:
			hand_tuning_ui ui;
			tunable_controller* tc;
	};

	hand_tuning::hand_tuning(ball::ptr ball, field::ptr field, controlled_team::ptr team) : movement_benchmark(ball, field, team), ui(this), tc(NULL) {
		reset_button = Gtk::manage(new Gtk::Button("Run"));
		reset_button->signal_clicked().connect(sigc::mem_fun(this,&hand_tuning::run));
		done = tasks.size();
		time_steps = 0;
	}

	hand_tuning::~hand_tuning() {
	}

	void hand_tuning::reset() {
		tc = tunable_controller::get_instance();
		time_steps = 0;
		done = tasks.size();
		ui.reset(tc);
	}

	void hand_tuning::run() {
		if (tc) {
			const std::vector<double>& params = ui.read_params();
			tc->set_params(params);
		}
		done = 0;
		time_steps = 0;
	}

	void hand_tuning::tick() {
		if (tc != tunable_controller::get_instance()) {
			reset();
		}
		movement_benchmark::tick();
	}

	class hand_tuning_factory : public strategy_factory {
		public:
			hand_tuning_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	hand_tuning_factory::hand_tuning_factory() : strategy_factory("Hand Tune & Move Benchmark") {
	}

	strategy::ptr hand_tuning_factory::create_strategy(xmlpp::Element *, ball::ptr ball, field::ptr field, controlled_team::ptr team) {
		strategy::ptr s(new hand_tuning(ball, field, team));
		return s;
	}

	hand_tuning_factory factory;

	strategy_factory &hand_tuning::get_factory() {
		return factory;
	}

}
