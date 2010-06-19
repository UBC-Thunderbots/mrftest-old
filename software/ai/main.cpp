#include "ai/window.h"
#include "ai/world/world.h"
#include "uicomponents/abstract_list_model.h"
#include "uicomponents/param.h"
#include "util/clocksource_timerfd.h"
#include "util/config.h"
#include "util/timestep.h"
#include "xbee/client/drive.h"
#include "xbee/client/lowlevel.h"
#include <algorithm>
#include <clocale>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <gtkmm.h>

namespace {
	/**
	 * A list model that allows the user to mark each robot as friendly or
	 * enemy.
	 */
	class team_customizer_model : public Glib::Object, public abstract_list_model {
		public:
			/**
			 * A column indicating the state of the robot.
			 */
			Gtk::TreeModelColumn<bool> friendly_column;

			/**
			 * A column showing the name of the robot.
			 */
			Gtk::TreeModelColumn<Glib::ustring> name_column;

			/**
			 * Constructs a new model.
			 *
			 * \param[in] conf the configuration file to show.
			 *
			 * \return the new model.
			 */
			static Glib::RefPtr<team_customizer_model> create(const config &conf) {
				Glib::RefPtr<team_customizer_model> p(new team_customizer_model(conf));
				return p;
			}

			/**
			 * \return a vector containing \c true if a robot is friendly or \c
			 * false if it is enemy.
			 */
			const std::vector<bool> &friendly() const {
				return friendly_;
			}

		private:
			const config &conf;
			std::vector<bool> friendly_;

			team_customizer_model(const config &conf) : Glib::ObjectBase(typeid(team_customizer_model)), conf(conf), friendly_(conf.robots().size(), true) {
				alm_column_record.add(friendly_column);
				alm_column_record.add(name_column);
			}

			unsigned int alm_rows() const {
				return conf.robots().size();
			}

			void alm_get_value(unsigned int row, unsigned int col, Glib::ValueBase &value) const {
				if (col == static_cast<unsigned int>(friendly_column.index())) {
					Glib::Value<bool> v;
					v.init(friendly_column.type());
					v.set(friendly_[row]);
					value.init(friendly_column.type());
					value = v;
				} else if (col == static_cast<unsigned int>(name_column.index())) {
					Glib::Value<Glib::ustring> v;
					v.init(name_column.type());
					v.set(conf.robots()[row].name);
					value.init(name_column.type());
					value = v;
				} else {
					std::abort();
				}
			}

			void alm_set_value(unsigned int row, unsigned int col, const Glib::ValueBase &value) {
				if (col == static_cast<unsigned int>(friendly_column.index())) {
					Glib::Value<bool> v;
					v.init(friendly_column.type());
					Glib::ValueBase &vb(v);
					vb = value;
					friendly_[row] = v.get();
				}
			}
	};

	class custom_team_builder : public Gtk::Window {
		public:
			custom_team_builder(config &conf) : conf(conf), model(team_customizer_model::create(conf)) {
				set_title("Thunderbots AI");
				set_default_size(200, 200);

				Gtk::TreeView *view = Gtk::manage(new Gtk::TreeView(model));
				view->get_selection()->set_mode(Gtk::SELECTION_SINGLE);
				view->append_column_editable("Friendly", model->friendly_column);
				view->append_column("Name", model->name_column);
				Gtk::ScrolledWindow *scroller = Gtk::manage(new Gtk::ScrolledWindow);
				scroller->add(*view);
				scroller->set_shadow_type(Gtk::SHADOW_IN);
				add(*scroller);

				show_all();
			}

		private:
			config &conf;
			Glib::RefPtr<team_customizer_model> model;

			bool on_delete_event(GdkEventAny *) {
				std::vector<uint64_t> to_remove;
				const std::vector<bool> &friendly(model->friendly());
				for (unsigned int i = 0; i < conf.robots().size(); ++i) {
					if (!friendly[i]) {
						to_remove.push_back(conf.robots()[i].address);
					}
				}
				std::for_each(to_remove.begin(), to_remove.end(), sigc::mem_fun(conf.robots(), &config::robot_set::remove));
				return false;
			}
	};

	int main_impl(int argc, char **argv) {
		std::setlocale(LC_ALL, "");

		Glib::OptionContext option_context;
		option_context.set_summary("Runs the Thunderbots control process.");

		Glib::OptionGroup option_group("thunderbots", "Control Process Options", "Show Control Process Options");

		Glib::OptionEntry all_entry;
		all_entry.set_long_name("all");
		all_entry.set_short_name('a');
		all_entry.set_description("Places all robots in the configuration file onto the friendly team");
		bool all = false;
		option_group.add_entry(all_entry, all);

		Glib::OptionEntry blue_entry;
		blue_entry.set_long_name("blue");
		blue_entry.set_short_name('b');
		blue_entry.set_description("Places all robots with blue lids onto the friendly team");
		bool blue = false;
		option_group.add_entry(blue_entry, blue);

		Glib::OptionEntry yellow_entry;
		yellow_entry.set_long_name("yellow");
		yellow_entry.set_short_name('y');
		yellow_entry.set_description("Places all robots with yellow lids onto the friendly team");
		bool yellow = false;
		option_group.add_entry(yellow_entry, yellow);

		Glib::OptionEntry custom_entry;
		custom_entry.set_long_name("custom");
		custom_entry.set_short_name('c');
		custom_entry.set_description("Allows a custom configuration of who is on the friendly team");
		bool custom = false;
		option_group.add_entry(custom_entry, custom);

		Glib::OptionEntry strategy_entry;
		strategy_entry.set_long_name("strategy");
		strategy_entry.set_description("Selects which strategy should be selected at startup");
		strategy_entry.set_arg_description("STRATEGY");
		Glib::ustring strategy_name;
		option_group.add_entry(strategy_entry, strategy_name);

		Glib::OptionEntry robot_controller_entry;
		robot_controller_entry.set_long_name("controller");
		robot_controller_entry.set_description("Selects which robot controller should be selected at startup");
		robot_controller_entry.set_arg_description("CONTROLLER");
		Glib::ustring robot_controller_name;
		option_group.add_entry(robot_controller_entry, robot_controller_name);

		Glib::OptionEntry ball_filter_entry;
		ball_filter_entry.set_long_name("ball-filter");
		ball_filter_entry.set_description("Selects which ball filter should be selected at startup");
		ball_filter_entry.set_arg_description("FILTER");
		Glib::ustring ball_filter_name;
		option_group.add_entry(ball_filter_entry, ball_filter_name);

		Glib::OptionEntry visualizer_entry;
		visualizer_entry.set_long_name("visualizer");
		visualizer_entry.set_short_name('v');
		visualizer_entry.set_description("Starts with the visualizer displayed");
		bool visualizer = false;
		option_group.add_entry(visualizer_entry, visualizer);

		Glib::OptionEntry minimize_entry;
		minimize_entry.set_long_name("minimize");
		minimize_entry.set_short_name('m');
		minimize_entry.set_description("Starts with the control window minimized");
		bool minimize = false;
		option_group.add_entry(minimize_entry, minimize);

		option_context.set_main_group(option_group);

		Gtk::Main app(argc, argv, option_context);

		if (argc != 1 || (!all && !blue && !yellow && !custom)) {
			std::cerr << option_context.get_help();
			return 1;
		}

		const bool options[4] = { all, blue, yellow, custom };
		if (std::count(options, options + sizeof(options) / sizeof(*options), true) > 1) {
			std::cerr << option_context.get_help();
			return 1;
		}

		bool refbox_yellow = false;
		config conf;
		if (all) {
			// Keep all robots in config.
		} else if (blue || yellow) {
			// Keep only blue or yellow robots.
			std::vector<uint64_t> to_remove;
			for (unsigned int i = 0; i < conf.robots().size(); ++i) {
				if (conf.robots()[i].yellow != yellow) {
					to_remove.push_back(conf.robots()[i].address);
				}
			}
			std::for_each(to_remove.begin(), to_remove.end(), sigc::mem_fun(conf.robots(), &config::robot_set::remove));
			refbox_yellow = yellow;
		} else if (custom) {
			custom_team_builder builder(conf);
			Gtk::Main::run(builder);
		} else {
			std::cerr << "WTF happened?\n";
			return 1;
		}

		xbee_lowlevel modem;

		std::vector<xbee_drive_bot::ptr> xbee_bots;
		for (unsigned int i = 0; i < conf.robots().size(); ++i) {
			xbee_bots.push_back(xbee_drive_bot::create(conf.robots()[i].name, conf.robots()[i].address, modem));
		}

		param::initialized(&conf);

		world::ptr the_world(world::create(conf, xbee_bots));
		if (refbox_yellow) {
			the_world->flip_refbox_colour();
		}

		if (!ball_filter_name.empty()) {
			ball_filter::map_type::const_iterator i = ball_filter::all().find(ball_filter_name.collate_key());
			if (i == ball_filter::all().end()) {
				std::cout << "There is no ball filter '" << ball_filter_name << "'.\n";
				return 1;
			}
			the_world->ball_filter(i->second);
		}

		clocksource_timerfd clk(UINT64_C(1000000000) / TIMESTEPS_PER_SECOND);

		ai the_ai(the_world, clk);

		if (!strategy_name.empty()) {
			strategy_factory::map_type::const_iterator i = strategy_factory::all().find(strategy_name.collate_key());
			if (i == strategy_factory::all().end()) {
				std::cout << "There is no strategy '" << strategy_name << "'.\n";
				return 1;
			}
			the_ai.set_strategy(i->second->create_strategy(the_world));
		}

		if (!robot_controller_name.empty()) {
			robot_controller_factory::map_type::const_iterator i = robot_controller_factory::all().find(robot_controller_name.collate_key());
			if (i == robot_controller_factory::all().end()) {
				std::cout << "There is no robot controller '" << robot_controller_name << "'.\n";
				return 1;
			}
			the_ai.set_robot_controller_factory(i->second);
		}

		ai_window win(the_ai, visualizer);

		if (minimize) {
			win.iconify();
		}

		clk.start();

		Gtk::Main::run(win);
		return 0;
	}
}

int main(int argc, char **argv) {
	try {
		return main_impl(argc, argv);
	} catch (const Glib::Exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (const std::exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}

