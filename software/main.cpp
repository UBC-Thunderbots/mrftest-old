#include "simulator/simulator.h"
#include "simulator/window.h"
#include "util/xml.h"
#include "world/config.h"
#include <iostream>
#include <getopt.h>
#include <gtkmm.h>
#include <libxml++/libxml++.h>



namespace {
	const char SHORT_OPTIONS[] = "wsfh";
	const option LONG_OPTIONS[] = {
		{"world", no_argument, 0, 'w'},
		{"simulator", no_argument, 0, 's'},
		{"sim", no_argument, 0, 's'},
		{"firmware", no_argument, 0, 'f'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	void usage(const char *app) {
		std::cerr << "Usage:\n\n" << app << " options\n\n";
		std::cerr << "-w\n--world\n\tRuns the \"real-world\" driver using XBee and cameras.\n\n";
		std::cerr << "-s\n--sim\n--simulator\n\tRuns the simulator.\n\n";
		std::cerr << "-f\n--firwmare\n\tRuns the firmware manager.\n\n";
		std::cerr << "-h\n--help\n\tDisplays this message.\n\n";
	}

	void simulate() {
		// Get the XML document.
		xmlpp::Document *xmldoc = config::get();

		// Get the root node, creating it if it doesn't exist.
		xmlpp::Element *xmlroot = xmldoc->get_root_node();
		if (!xmlroot || xmlroot->get_name() != "thunderbots") {
			xmlroot = xmldoc->create_root_node("thunderbots");
		}
		xmlutil::strip(xmlroot);

		// Get the simulator node, creating it if it doesn't exist.
		xmlpp::Element *xmlsim = xmlutil::strip(xmlutil::get_only_child(xmlroot, "simulator"));

		// Create the simulator object.
		simulator sim(xmlsim);

		// Create the UI.
		simulator_window win(sim);

		// Go go go!
		Gtk::Main::run();
	}
}

int main(int argc, char **argv) {
	// Initialize GTK+.
	Gtk::Main mn(argc, argv);

	// Parse options.
	unsigned int do_world = 0, do_sim = 0, do_firmware = 0, do_help = 0;
	int ch;
	while ((ch = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, 0)) != -1)
		switch (ch) {
			case 'w':
				do_world++;
				break;

			case 's':
				do_sim++;
				break;

			case 'f':
				do_firmware++;
				break;

			default:
				do_help++;
				break;
		}

	// Check for legal combinations of options.
	if (do_help || (do_world + do_sim + do_firmware != 1)) {
		usage(argv[0]);
		return 1;
	}

	// Load the configuration file.
	config::get();

	// Dispatch and do real work.
	if (do_world) {
		std::cerr << "World operation is not implemented yet.\n";
	} else if (do_sim) {
		simulate();
	} else if (do_firmware) {
		std::cerr << "Firmware manager is not implemented yet.\n";
	}

	// The configuration file might recently have been dirtied but not flushed
	// yet if the user quit the application immediately after the dirtying.
	// Flush the configuration file now.
	config::force_save();

	return 0;
}

