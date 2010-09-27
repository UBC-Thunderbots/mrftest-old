#include "ai/ai.h"
#include "ai/window.h"
#include "ai/backend/backend.h"
#include "uicomponents/abstract_list_model.h"
#include "uicomponents/param.h"
#include "util/clocksource_timerfd.h"
#include "util/config.h"
#include "util/timestep.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <gtkmm.h>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <stdint.h>
#include <vector>

namespace {
	struct WithBackendClosure {
		const Glib::ustring &coach_name;
		const Glib::ustring &robot_controller_name;
		const Glib::ustring &ball_filter_name;
		bool minimize;
		AI::BE::Backend::FieldEnd defending_end;
		AI::BE::Backend::Colour friendly_colour;

		WithBackendClosure(const Glib::ustring &coach_name, const Glib::ustring &robot_controller_name, const Glib::ustring &ball_filter_name, bool minimize, AI::BE::Backend::FieldEnd defending_end, AI::BE::Backend::Colour friendly_colour) : coach_name(coach_name), robot_controller_name(robot_controller_name), ball_filter_name(ball_filter_name), minimize(minimize), defending_end(defending_end), friendly_colour(friendly_colour) {
		}
	};

	void main_impl_with_backend(AI::BE::Backend &backend, const WithBackendClosure &wbc) {
		backend.defending_end() = wbc.defending_end;
		backend.friendly_colour() = wbc.friendly_colour;

		if (!wbc.ball_filter_name.empty()) {
			typedef AI::BF::BallFilter::Map Map;
			const Map &m = AI::BF::BallFilter::all();
			const Map::const_iterator &i = m.find(wbc.ball_filter_name.collate_key());
			if (i == m.end()) {
				throw std::runtime_error(Glib::ustring::compose("There is no ball filter '%1'.", wbc.ball_filter_name));
			}
			backend.ball_filter() = i->second;
		}

		AI::AIPackage ai(backend);

		if (!wbc.coach_name.empty()) {
			typedef AI::Coach::CoachFactory::Map Map;
			const Map &m = AI::Coach::CoachFactory::all();
			const Map::const_iterator &i = m.find(wbc.coach_name.collate_key());
			if (i == m.end()) {
				throw std::runtime_error(Glib::ustring::compose("There is no coach '%1'.", wbc.coach_name));
			}
			ai.coach = i->second->create_coach(backend);
		}

		if (!wbc.robot_controller_name.empty()) {
			typedef AI::RC::RobotControllerFactory::Map Map;
			const Map &m = AI::RC::RobotControllerFactory::all();
			const Map::const_iterator &i = m.find(wbc.robot_controller_name.collate_key());
			if (i == m.end()) {
				throw std::runtime_error(Glib::ustring::compose("There is no robot controller '%1'.", wbc.robot_controller_name));
			}
			ai.robot_controller_factory = i->second;
		}

		AI::Window win(ai);

		if (wbc.minimize) {
			win.iconify();
		}

		Gtk::Main::run(win);
	}

	Glib::ustring choose_backend() {
		Gtk::Dialog dlg("Thunderbots AI", true);
		dlg.get_vbox()->pack_start(*Gtk::manage(new Gtk::Label("Select a backend:")), Gtk::PACK_SHRINK);
		Gtk::ComboBoxText combo;
		typedef AI::BE::BackendFactory::Map Map;
		const Map &m = AI::BE::BackendFactory::all();
		for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
			combo.append_text(i->second->name);
		}
		dlg.get_vbox()->pack_start(combo, Gtk::PACK_SHRINK);
		dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
		dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		dlg.set_default_response(Gtk::RESPONSE_OK);
		dlg.show_all();
		const int resp = dlg.run();
		if (resp != Gtk::RESPONSE_OK) {
			return "";
		}
		return combo.get_active_text();
	}

	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		// Seed the PRNG.
		std::srand(std::time(0));

		// Parse the command-line arguments.
		Glib::OptionContext option_context;
		option_context.set_summary("Runs the Thunderbots control process.");

		Glib::OptionGroup option_group("thunderbots", "Control Process Options", "Show Control Process Options");

		Glib::OptionEntry list_entry;
		list_entry.set_long_name("list");
		list_entry.set_short_name('l');
		list_entry.set_description("Lists available components of each type");
		bool list = false;
		option_group.add_entry(list_entry, list);

		Glib::OptionEntry blue_entry;
		blue_entry.set_long_name("blue");
		blue_entry.set_short_name('b');
		blue_entry.set_description("Controls blue robots (default is yellow)");
		bool blue = false;
		option_group.add_entry(blue_entry, blue);

		Glib::OptionEntry east_entry;
		east_entry.set_long_name("east");
		east_entry.set_short_name('e');
		east_entry.set_description("Defends east goal (default is west)");
		bool east = false;
		option_group.add_entry(east_entry, east);

		Glib::OptionEntry backend_entry;
		backend_entry.set_long_name("backend");
		backend_entry.set_description("Selects which backend should be used");
		backend_entry.set_arg_description("BACKEND");
		Glib::ustring backend_name;
		option_group.add_entry(backend_entry, backend_name);

		Glib::OptionEntry coach_entry;
		coach_entry.set_long_name("coach");
		coach_entry.set_description("Selects which coach should be selected at startup");
		coach_entry.set_arg_description("COACH");
		Glib::ustring coach_name;
		option_group.add_entry(coach_entry, coach_name);

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

		Glib::OptionEntry minimize_entry;
		minimize_entry.set_long_name("minimize");
		minimize_entry.set_short_name('m');
		minimize_entry.set_description("Starts with the control window minimized");
		bool minimize = false;
		option_group.add_entry(minimize_entry, minimize);

		option_context.set_main_group(option_group);

		Gtk::Main app(argc, argv, option_context);

		if (list) {
			{
				std::cerr << "The following backends are available:\n";
				typedef AI::BE::BackendFactory::Map Map;
				const Map &m = AI::BE::BackendFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					std::cerr << i->second->name << '\n';
				}
				std::cerr << '\n';
			}
			{
				std::cerr << "The following coaches are available:\n";
				typedef AI::Coach::CoachFactory::Map Map;
				const Map &m = AI::Coach::CoachFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					std::cerr << i->second->name << '\n';
				}
				std::cerr << '\n';
			}
			{
				std::cerr << "The following robot controllers are available:\n";
				typedef AI::RC::RobotControllerFactory::Map Map;
				const Map &m = AI::RC::RobotControllerFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					std::cerr << i->second->name << '\n';
				}
				std::cerr << '\n';
			}
			{
				std::cerr << "The following ball filters are available:\n";
				typedef AI::BF::BallFilter::Map Map;
				const Map &m = AI::BF::BallFilter::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					std::cerr << i->second->name << '\n';
				}
				std::cerr << '\n';
			}
			return 0;
		}

		if (argc != 1) {
			std::cerr << option_context.get_help();
			return 1;
		}

		// Load the configuration file.
		Config conf;

		// Initialize the parameters from the configuration file.
		Param::initialized(&conf);

		// Create the backend.
		WithBackendClosure wbc(coach_name, robot_controller_name, ball_filter_name, minimize, east ? AI::BE::Backend::EAST : AI::BE::Backend::WEST, blue ? AI::BE::Backend::BLUE : AI::BE::Backend::YELLOW);
		if (!backend_name.size()) {
			backend_name = choose_backend();
			if (!backend_name.size()) {
				return 0;
			}
		}
		typedef AI::BE::BackendFactory::Map Map;
		const Map &bem = AI::BE::BackendFactory::all();
		const Map::const_iterator &be = bem.find(backend_name.collate_key());
		if (be == bem.end()) {
			throw std::runtime_error(Glib::ustring::compose("There is no backend '%1'.", backend_name));
		}
		be->second->create_backend(conf, sigc::bind(sigc::ptr_fun(&main_impl_with_backend), wbc));

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

