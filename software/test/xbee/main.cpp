#include "main.h"
#include "test/xbee/launcher.h"
#include "util/annunciator.h"
#include "util/config.h"
#include "xbee/dongle.h"
#include <iostream>
#include <locale>
#include <gtkmm/main.h>

int app_main(int argc, char **argv) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

	// Parse the command-line arguments.
	Glib::OptionContext option_context;
	option_context.set_summary("Allows testing robot subsystems.");

	// Load the configuration file.
	Config::load();

	// Initialize GTK.
	Gtk::Main m(argc, argv, option_context);
	if (argc != 1) {
		std::cout << option_context.get_help();
		return 1;
	}

	// Find and enable the dongle.
	std::cout << "Finding dongle... " << std::flush;
	XBeeDongle dongle;
	std::cout << "OK\nEnabling radios... " << std::flush;
	dongle.enable();
	std::cout << "OK\n";

	// Create the window.
	TesterLauncher win(dongle);
	Gtk::Main::run(win);

	return 0;
}

