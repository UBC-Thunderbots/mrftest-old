#include "ai/ai.h"
#include "ai/logger.h"
#include "ai/window.h"
#include "ai/backend/backend.h"
#include "uicomponents/abstract_list_model.h"
#include "util/clocksource_timerfd.h"
#include "util/param.h"
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
		const Glib::ustring &high_level_name;
		const Glib::ustring &robot_controller_name;
		const Glib::ustring &ball_filter_name;
		bool minimize;
		AI::BE::Backend::FieldEnd defending_end;
		AI::Common::Team::Colour friendly_colour;

		WithBackendClosure(const Glib::ustring &high_level_name, const Glib::ustring &robot_controller_name, const Glib::ustring &ball_filter_name, bool minimize, AI::BE::Backend::FieldEnd defending_end, AI::Common::Team::Colour friendly_colour) : high_level_name(high_level_name), robot_controller_name(robot_controller_name), ball_filter_name(ball_filter_name), minimize(minimize), defending_end(defending_end), friendly_colour(friendly_colour) {
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

		if (!wbc.high_level_name.empty()) {
			typedef AI::HL::HighLevelFactory::Map Map;
			const Map &m = AI::HL::HighLevelFactory::all();
			const Map::const_iterator &i = m.find(wbc.high_level_name.collate_key());
			if (i == m.end()) {
				throw std::runtime_error(Glib::ustring::compose("There is no high level '%1'.", wbc.high_level_name));
			}
			ai.high_level = i->second->create_high_level(backend);
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

		AI::Logger logger(ai);
		try {
			AI::Window win(ai);

			if (wbc.minimize) {
				win.iconify();
			}

			Gtk::Main::run(win);
		} catch (const Glib::Exception &exp) {
			logger.end_with_exception(exp.what());
			throw;
		} catch (const std::exception &exp) {
			logger.end_with_exception(exp.what());
			throw;
		} catch (...) {
			logger.end_with_exception("Unknown exception");
			throw;
		}
	}

	Glib::ustring choose_backend() {
		Gtk::Dialog dlg("Thunderbots AI", true);
		dlg.get_vbox()->pack_start(*Gtk::manage(new Gtk::Label("Select a backend:")), Gtk::PACK_SHRINK);
		Gtk::ComboBoxText combo;
		typedef AI::BE::BackendFactory::Map Map;
		const Map &m = AI::BE::BackendFactory::all();
		for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
			combo.append_text(i->second->name());
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

		// Seed the PRNGs.
		std::srand(static_cast<unsigned int>(std::time(0)));
		srand48(static_cast<long>(std::time(0)));

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
		backend_entry.set_arg_description("BACKEND[/PARAM=VALUE[,...]]");
		Glib::ustring backend_name_and_params;
		option_group.add_entry(backend_entry, backend_name_and_params);

		Glib::OptionEntry high_level_entry;
		high_level_entry.set_long_name("hl");
		high_level_entry.set_description("Selects which high level should be selected at startup");
		high_level_entry.set_arg_description("HIGHLEVEL");
		Glib::ustring high_level_name;
		option_group.add_entry(high_level_entry, high_level_name);

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
					std::cerr << i->second->name() << '\n';
				}
				std::cerr << '\n';
			}
			{
				std::cerr << "The following high levels are available:\n";
				typedef AI::HL::HighLevelFactory::Map Map;
				const Map &m = AI::HL::HighLevelFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					std::cerr << i->second->name() << '\n';
				}
				std::cerr << '\n';
			}
			{
				std::cerr << "The following robot controllers are available:\n";
				typedef AI::RC::RobotControllerFactory::Map Map;
				const Map &m = AI::RC::RobotControllerFactory::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					std::cerr << i->second->name() << '\n';
				}
				std::cerr << '\n';
			}
			{
				std::cerr << "The following ball filters are available:\n";
				typedef AI::BF::BallFilter::Map Map;
				const Map &m = AI::BF::BallFilter::all();
				for (Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					std::cerr << i->second->name() << '\n';
				}
				std::cerr << '\n';
			}
			return 0;
		}

		if (argc != 1) {
			std::cerr << option_context.get_help();
			return 1;
		}

		// Initialize the parameters.
		ParamTreeNode::root()->initialize();
		ParamTreeNode::load_all();

		// Enable the use of the siren for annunciator messages.
		Annunciator::activate_siren();

		// Create the backend.
		WithBackendClosure wbc(high_level_name, robot_controller_name, ball_filter_name, minimize, east ? AI::BE::Backend::EAST : AI::BE::Backend::WEST, blue ? AI::Common::Team::BLUE : AI::Common::Team::YELLOW);
		if (!backend_name_and_params.size()) {
			backend_name_and_params = choose_backend();
			if (!backend_name_and_params.size()) {
				return 0;
			}
		}
		Glib::ustring backend_name;
		std::multimap<Glib::ustring, Glib::ustring> backend_params;
		{
			Glib::ustring::size_type slash_index = backend_name_and_params.find('/');
			if (slash_index == Glib::ustring::npos) {
				backend_name = backend_name_and_params;
			} else {
				backend_name = backend_name_and_params.substr(0, slash_index);
				Glib::ustring::size_type start_param_name = slash_index + 1;
				while (start_param_name < backend_name_and_params.size()) {
					Glib::ustring::size_type equals = backend_name_and_params.find('=', start_param_name);
					if (equals == Glib::ustring::npos) {
						throw std::runtime_error("Invalid parameter format, must be comma-separated sequence of KEY=VALUE");
					}
					const Glib::ustring &param_name = backend_name_and_params.substr(start_param_name, equals - start_param_name);
					Glib::ustring::size_type comma = backend_name_and_params.find(',', equals);
					if (comma == Glib::ustring::npos) {
						comma = backend_name_and_params.size();
					}
					const Glib::ustring &param_value = backend_name_and_params.substr(equals + 1, comma - (equals + 1));
					backend_params.insert(std::make_pair(param_name, param_value));
					start_param_name = comma + 1;
				}
			}
		}
		typedef AI::BE::BackendFactory::Map Map;
		const Map &bem = AI::BE::BackendFactory::all();
		const Map::const_iterator &be = bem.find(backend_name.collate_key());
		if (be == bem.end()) {
			throw std::runtime_error(Glib::ustring::compose("There is no backend '%1'.", backend_name));
		}
		be->second->create_backend(backend_params, sigc::bind(sigc::ptr_fun(&main_impl_with_backend), wbc));

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

