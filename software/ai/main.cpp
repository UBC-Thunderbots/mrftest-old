#include "main.h"
#include "ai/ai.h"
#include "ai/logger.h"
#include "ai/setup.h"
#include "ai/window.h"
#include "ai/backend/backend.h"
#include "uicomponents/abstract_list_model.h"
#include "util/annunciator.h"
#include "util/config.h"
#include "util/main_loop.h"
#include "util/param.h"
#include "util/random.h"
#include "util/timestep.h"
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <vector>
#include <glibmm/exception.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/dialog.h>
#include <gtkmm/label.h>
#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace {
	Glib::ustring choose_backend() {
		Gtk::Dialog dlg(u8"Thunderbots AI", true);
		Gtk::Label label(u8"Select a backend:");
		dlg.get_vbox()->pack_start(label, Gtk::PACK_SHRINK);
		Gtk::ComboBoxText combo;
		typedef AI::BE::BackendFactory::Map Map;
		for (const Map::value_type &i : AI::BE::BackendFactory::all()) {
			combo.append(i.second->name());
		}
		dlg.get_vbox()->pack_start(combo, Gtk::PACK_SHRINK);
		dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
		dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
		dlg.set_default_response(Gtk::RESPONSE_OK);
		dlg.show_all();
		const int resp = dlg.run();
		if (resp != Gtk::RESPONSE_OK) {
			return u8"";
		}
		return combo.get_active_text();
	}
}

int app_main(int argc, char **argv) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

	// Seed the PRNGs.
	Random::seed();

	// Parse the command-line arguments.
	AI::Setup setup;
	Glib::OptionContext option_context;
	option_context.set_summary(u8"Runs the Thunderbots control process.");

	Glib::OptionGroup option_group(u8"thunderbots", u8"Control Process Options", u8"Show Control Process Options");

	Glib::OptionEntry list_entry;
	list_entry.set_long_name(u8"list");
	list_entry.set_short_name('l');
	list_entry.set_description(u8"Lists available components of each type");
	bool list = false;
	option_group.add_entry(list_entry, list);

	Glib::OptionEntry blue_entry;
	blue_entry.set_long_name(u8"blue");
	blue_entry.set_short_name('b');
	blue_entry.set_description(u8"Controls blue robots (default is same as last time)");
	bool blue = false;
	option_group.add_entry(blue_entry, blue);

	Glib::OptionEntry yellow_entry;
	yellow_entry.set_long_name(u8"yellow");
	yellow_entry.set_short_name('y');
	yellow_entry.set_description(u8"Controls yellow robots (default is same as last time)");
	bool yellow = false;
	option_group.add_entry(yellow_entry, yellow);

	Glib::OptionEntry east_entry;
	east_entry.set_long_name(u8"east");
	east_entry.set_short_name('e');
	east_entry.set_description(u8"Defends east goal (default is same as last time)");
	bool east = false;
	option_group.add_entry(east_entry, east);

	Glib::OptionEntry west_entry;
	west_entry.set_long_name(u8"west");
	west_entry.set_short_name('w');
	west_entry.set_description(u8"Defends west goal (default is same as last time)");
	bool west = false;
	option_group.add_entry(west_entry, west);

	Glib::OptionEntry multicast_interface_entry;
	multicast_interface_entry.set_long_name(u8"interface");
	multicast_interface_entry.set_description(u8"Overrides the kernel's default choice of network interface on which to receive multicast packets");
	multicast_interface_entry.set_arg_description(u8"IFNAME");
	Glib::ustring multicast_interface_name;
	option_group.add_entry(multicast_interface_entry, multicast_interface_name);

	Glib::OptionEntry backend_entry;
	backend_entry.set_long_name(u8"backend");
	backend_entry.set_description(u8"Selects which backend should be used");
	backend_entry.set_arg_description(u8"BACKEND");
	Glib::ustring backend_name;
	option_group.add_entry(backend_entry, backend_name);

	Glib::OptionEntry disable_camera_entry;
	disable_camera_entry.set_long_name(u8"disable-camera");
	disable_camera_entry.set_description(u8"Disables reception of packets from a particular camera");
	disable_camera_entry.set_arg_description(u8"ID");
	std::vector<Glib::ustring> disable_camera_strings;
	option_group.add_entry(disable_camera_entry, disable_camera_strings);

	Glib::OptionEntry high_level_entry;
	high_level_entry.set_long_name(u8"hl");
	high_level_entry.set_description(u8"Selects which high level should be selected at startup");
	high_level_entry.set_arg_description(u8"HIGHLEVEL");
	option_group.add_entry(high_level_entry, setup.high_level_name);

	Glib::OptionEntry minimize_entry;
	minimize_entry.set_long_name(u8"minimize");
	minimize_entry.set_short_name('m');
	minimize_entry.set_description(u8"Starts with the control window minimized");
	bool minimize = false;
	option_group.add_entry(minimize_entry, minimize);

	option_context.set_main_group(option_group);

	Gtk::Main app(argc, argv, option_context);

	if (list) {
		{
			std::cerr << "The following backends are available:\n";
			typedef AI::BE::BackendFactory::Map Map;
			for (const Map::value_type &i : AI::BE::BackendFactory::all()) {
				std::cerr << i.second->name() << '\n';
			}
			std::cerr << '\n';
		}
		{
			std::cerr << "The following high levels are available:\n";
			typedef AI::HL::HighLevelFactory::Map Map;
			for (const Map::value_type &i : AI::HL::HighLevelFactory::all()) {
				std::cerr << i.second->name() << '\n';
			}
			std::cerr << '\n';
		}
		return 0;
	}

	if (argc != 1 || (east && west) || (yellow && blue)) {
		std::cerr << option_context.get_help();
		return 1;
	}

	std::vector<bool> disable_cameras;
	for (auto &i : disable_camera_strings) {
		unsigned long ul = std::stoul(Glib::locale_from_utf8(i), nullptr, 0);
		if (disable_cameras.size() <= ul) {
			disable_cameras.resize(ul + 1, false);
		}
		disable_cameras[ul] = true;
	}

	// Initialize the parameters.
	ParamTreeNode::root()->initialize();

	// Load the configuration file.
	Config::load();
	ParamTreeNode::load_all();

	// Create the backend.
	if (east) {
		setup.defending_end = AI::BE::Backend::FieldEnd::EAST;
	} else if (west) {
		setup.defending_end = AI::BE::Backend::FieldEnd::WEST;
	}
	if (yellow) {
		setup.friendly_colour = AI::Common::Colour::YELLOW;
	} else if (blue) {
		setup.friendly_colour = AI::Common::Colour::BLUE;
	}
	int multicast_interface_index = 0;
	if (!multicast_interface_name.empty()) {
		multicast_interface_index = static_cast<int>(if_nametoindex(Glib::locale_from_utf8(multicast_interface_name).c_str()));
		if (!multicast_interface_index) {
			throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose(u8"Interface “%1” not found.", multicast_interface_name)).c_str());
		}
	}
	setup.save();
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
		throw std::runtime_error(Glib::ustring::compose(u8"There is no backend “%1”.", backend_name));
	}
	const std::unique_ptr<AI::BE::Backend> &beptr = be->second->create_backend(disable_cameras, multicast_interface_index);
	AI::BE::Backend &backend = *beptr.get();

	// Bring in configuration parameters.
	backend.defending_end() = setup.defending_end;
	backend.friendly_colour() = setup.friendly_colour;

	// Instantiate the AI.
	AI::AIPackage ai(backend);

	// Select the last selected high level.
	if (!setup.high_level_name.empty()) {
		typedef AI::HL::HighLevelFactory::Map Map;
		const Map &m = AI::HL::HighLevelFactory::all();
		const Map::const_iterator &i = m.find(setup.high_level_name.collate_key());
		if (i != m.end()) {
			ai.high_level = i->second->create_high_level(AI::HL::W::World(backend));
		}
	}

	// Start logging.
	AI::Logger logger(ai);
	backend.log_to(logger);

	// Run the AI.
	try {
		AI::Window win(ai);

		if (minimize) {
			win.iconify();
		}

		MainLoop::run(win);
	} catch (const Glib::Exception &exp) {
		logger.end_with_exception(exp.what());
		throw;
	} catch (const std::exception &exp) {
		logger.end_with_exception(exp.what());
		throw;
	} catch (...) {
		logger.end_with_exception(u8"Unknown exception");
		throw;
	}

	return 0;
}

