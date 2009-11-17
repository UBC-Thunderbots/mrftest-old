#include "firmware/window.h"
#include "simulator/window.h"
#include "util/args.h"
#include "util/clocksource_quick.h"
#include "util/clocksource_timerfd.h"
#include "util/xml.h"
#include "world/config.h"
#include "world/timestep.h"
#include <iostream>
#include <gtkmm.h>
#include <getopt.h>

namespace {
	const char SHORT_OPTIONS[] = "wsqfh";
	const option LONG_OPTIONS[] = {
		{"world", no_argument, 0, 'w'},
		{"simulator", no_argument, 0, 's'},
		{"sim", no_argument, 0, 's'},
		{"quick", no_argument, 0, 'q'},
		{"firmware", no_argument, 0, 'f'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	void usage(const char *app) {
		std::cerr << "Usage:\n\n" << app << " options\n\n";
		std::cerr << "-w\n--world\n\tRuns the \"real-world\" driver using XBee and cameras.\n\n";
		std::cerr << "-s\n--sim\n--simulator\n\tRuns the simulator.\n\n";
		std::cerr << "-q\n--quick\n\tRuns in maximum-speed mode (only applicable to simulator).\n\n";
		std::cerr << "-f\n--firwmare\n\tRuns the firmware manager.\n\n";
		std::cerr << "-h\n--help\n\tDisplays this message.\n\n";
	}

	class fps_reporter : public sigc::trackable {
		public:
			fps_reporter(clocksource &simclock, clocksource &uiclock, simulator_window &win) : simticks(0), uiticks(0), win(win) {
				simclock.signal_tick().connect(sigc::mem_fun(*this, &fps_reporter::simtick));
				uiclock.signal_tick().connect(sigc::mem_fun(*this, &fps_reporter::uitick));
			}

		private:
			void simtick() {
				simticks++;
			}

			void uitick() {
				if (++uiticks == TIMESTEPS_PER_SECOND) {
					win.show_fps(simticks);
					simticks = uiticks = 0;
				}
			}

			unsigned int simticks, uiticks;
			simulator_window &win;
	};

	void simulate(clocksource &simclock, clocksource &uiclock) {
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
		simulator sim(xmlsim, simclock);

		// Create the UI.
		simulator_window win(sim, uiclock);

		// Create the frame rate reporter.
		fps_reporter fpsr(simclock, uiclock, win);

		// Go go go!
		Gtk::Main::run();
	}

	void manage_firmware() {
		// Get the XML document.
		xmlpp::Document *xmldoc = config::get();

		// Get the root node, creating it if it doesn't exist.
		xmlpp::Element *xmlroot = xmldoc->get_root_node();
		if (!xmlroot || xmlroot->get_name() != "thunderbots") {
			xmlroot = xmldoc->create_root_node("thunderbots");
		}
		xmlutil::strip(xmlroot);

		// Get the world node, creating it if it doesn't exist.
		xmlpp::Element *xmlworld = xmlutil::strip(xmlutil::get_only_child(xmlroot, "world"));

		// Create the XBee object.
		xbee modem;

		// Create the UI.
		firmware_window win(modem, xmlworld);

		// Go go go!
		Gtk::Main::run();
	}
}

int main(int argc, char **argv) {
	// Save raw arguments.
	args::argc = argc;
	args::argv = argv;

	// Initialize GTK+.
	Gtk::Main mn(argc, argv);

	// Parse options.
	unsigned int do_world = 0, do_sim = 0, do_quick = 0, do_firmware = 0, do_help = 0;
	int ch;
	while ((ch = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, 0)) != -1)
		switch (ch) {
			case 'w':
				do_world++;
				break;

			case 's':
				do_sim++;
				break;

			case 'q':
				do_quick++;
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
	if (do_quick > 1 || (do_quick && !do_sim)) {
		usage(argv[0]);
		return 1;
	}

	// Load the configuration file.
	config::get();

	// Dispatch and do real work.
	if (do_world) {
		std::cerr << "World operation is not implemented yet.\n";
	} else if (do_sim) {
		clocksource_timerfd time_clk(1000000000ULL / TIMESTEPS_PER_SECOND);
		time_clk.start();
		if (do_quick) {
			clocksource_quick quick_clk;
			quick_clk.start();
			simulate(quick_clk, time_clk);
		} else {
			simulate(time_clk, time_clk);
		}
	} else if (do_firmware) {
		manage_firmware();
	}

	// The configuration file might recently have been dirtied but not flushed
	// yet if the user quit the application immediately after the dirtying.
	// Flush the configuration file now.
	config::force_save();

	return 0;
}

