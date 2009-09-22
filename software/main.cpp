#include "simulator/window.h"
#include "world/config.h"
#include <iostream>
#include <getopt.h>
#include <gtkmm.h>



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
		config::get();
		simulator_window win;
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

	if (do_help || (do_world + do_sim + do_firmware != 1)) {
		usage(argv[0]);
		return 1;
	}

	if (do_world) {
		std::cerr << "World operation is not implemented yet.\n";
	} else if (do_sim) {
		simulate();
	} else if (do_firmware) {
		std::cerr << "Firmware manager is not implemented yet.\n";
	}

	return 0;
}

