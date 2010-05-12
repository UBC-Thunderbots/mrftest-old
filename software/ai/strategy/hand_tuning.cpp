#include "ai/strategy/movement_benchmark.h"
#include "robot_controller/tunable_controller.h"
#include "util/stochastic_local_search.h"

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
				set_title("Hand Tuning");
				//vbox.pack_start(button, true, true);
				vbox.add(button);
				button.signal_clicked().connect(sigc::mem_fun(this, &hand_tuning_ui::run));
				button.set_label("Run!");
				add(vbox);
			}

			~hand_tuning_ui() {
				for (size_t i = 0; i < entries.size(); ++i) {
					vbox.remove(*entries[i]);
					delete entries[i];
				}
			}

			void run();

			/// Resets the parameters on the ui.
			void reset_params(const std::vector<double>& mini, const std::vector<double>& maxi, const std::vector<double>& vals) {
				for (size_t i = 0; i < entries.size(); ++i) {
					vbox.remove(*entries[i]);
					delete entries[i];
				}
				entries.resize(vals.size());
				for (size_t i = 0; i < entries.size(); ++i) {
					entries[i] = new Gtk::Entry();
					vbox.add(*entries[i]);
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
			Gtk::VBox vbox;
			Gtk::Button button;
			std::vector<Gtk::Entry*> entries;
			hand_tuning* ht;
	};

	class hand_tuning : public movement_benchmark {
		public:
			hand_tuning(ball::ptr ball, field::ptr field, controlled_team::ptr team);
			~hand_tuning();
			strategy_factory &get_factory();
			// void tick();
			void run();
			void reset();
		private:
			tunable_controller* tc;
			hand_tuning_ui ui;
	};

	hand_tuning::hand_tuning(ball::ptr ball, field::ptr field, controlled_team::ptr team) : movement_benchmark(ball, field, team), ui(this) {
		reset_button = Gtk::manage(new Gtk::Button("Reset"));
		reset_button->signal_clicked().connect(sigc::mem_fun(this,&hand_tuning::reset));
	}

	hand_tuning::~hand_tuning() {
	}

	void hand_tuning_ui::run() {
		ht->run();
	}

	void hand_tuning::reset() {
		tc = tunable_controller::controller_instance;
		if (tc == NULL) return;
		time_steps = 0;
		done = tasks.size();
		ui.reset_params(tc->get_params_min(), tc->get_params_max(), tc->get_params());
	}

	void hand_tuning::run() {
		const std::vector<double>& params = ui.read_params();
		tc->set_params(params);
		done = 0;
		time_steps = 0;
	}

	class hand_tuning_factory : public strategy_factory {
		public:
			hand_tuning_factory();
			strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team);
	};

	hand_tuning_factory::hand_tuning_factory() : strategy_factory("Hand Tuning") {
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
