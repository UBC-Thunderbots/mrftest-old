#include "firmware/window.h"
#include "log/tools/tool_launcher.h"
#include "sim/window.h"
#include "tester/window.h"
#include "util/args.h"
#include "util/clocksource_quick.h"
#include "util/clocksource_timerfd.h"
#include "util/config.h"
#include "util/xml.h"
#include "world/timestep.h"
#include "xbee/packettypes.h"
#include "xbee/xbee.h"
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <gtkmm.h>
#include <getopt.h>

namespace {
	const char SHORT_OPTIONS[] = "wsqlftmh";
	const option LONG_OPTIONS[] = {
		{"world", no_argument, 0, 'w'},
		{"simulator", no_argument, 0, 's'},
		{"sim", no_argument, 0, 's'},
		{"quick", no_argument, 0, 'q'},
		{"log-viewer", no_argument, 0, 'l'},
		{"log", no_argument, 0, 'l'},
		{"firmware", no_argument, 0, 'f'},
		{"test", no_argument, 0, 't'},
		{"tester", no_argument, 0, 't'},
		{"modeminfo", no_argument, 0, 'm'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	void usage(const char *app) {
		std::cerr << "Usage:\n\n" << app << " options\n\n";
		std::cerr << "-w\n--world\n\tRuns the \"real-world\" driver using XBee and cameras.\n\n";
		std::cerr << "-s\n--sim\n--simulator\n\tRuns the simulator.\n\n";
		std::cerr << "-q\n--quick\n\tRuns in maximum-speed mode (only applicable to simulator).\n\n";
		std::cerr << "-l\n--log\n--log-viewer\n\tRuns the log replayer.\n\n";
		std::cerr << "-f\n--firwmare\n\tRuns the firmware manager.\n\n";
		std::cerr << "-t\n--test\n--tester\n\tRuns the robot tester.\n\n";
		std::cerr << "-m\n--modeminfo\n\tDisplays information about the attached XBee.\n\n";
		std::cerr << "-h\n--help\n\tDisplays this message.\n\n";
	}

	class fps_reporter : public sigc::trackable {
		public:
			fps_reporter(clocksource &simclock, clocksource &uiclock, simulator_window &win) : simticks(0), uiticks(0), win(win) {
				simclock.signal_tick().connect(sigc::mem_fun(this, &fps_reporter::simtick));
				uiclock.signal_tick().connect(sigc::mem_fun(this, &fps_reporter::uitick));
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
		simulator_window win(sim, simclock, uiclock);

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

	void run_tester() {
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
		tester_window win(modem, xmlworld);

		// Go go go!
		Gtk::Main::run();
	}

	struct run_modeminfo_info {
		unsigned int stage;
		uint8_t serial[8];
	};

	bool run_modeminfo_send_request(xbee *modem, run_modeminfo_info *info) {
		xbeepacket::AT_REQUEST<0> req;
		req.apiid = xbeepacket::AT_REQUEST_APIID;
		req.frame = 'A';
		switch (info->stage) {
			case 0:
				req.command[0] = 'S';
				req.command[1] = 'H';
				break;

			case 1:
				req.command[0] = 'S';
				req.command[1] = 'L';
				break;
		}
		modem->send(&req, sizeof(req));
		return true;
	}

	void run_modeminfo_handle_packet(const void *data, std::size_t length, xbee *modem, run_modeminfo_info *info) {
		if (length < sizeof(xbeepacket::AT_RESPONSE)) {
			return;
		}
		const xbeepacket::AT_RESPONSE &packet = *static_cast<const xbeepacket::AT_RESPONSE *>(data);
		if (packet.apiid != xbeepacket::AT_RESPONSE_APIID) {
			return;
		}
		if (packet.frame != 'A') {
			return;
		}
		switch (info->stage) {
			case 0:
				if (packet.command[0] != 'S' || packet.command[1] != 'H') {
					return;
				}
				if (packet.status != xbeepacket::AT_RESPONSE_STATUS_OK) {
					std::cerr << "AT command returned error " << packet.status << '\n';
					Gtk::Main::quit();
					return;
				}
				if (length != sizeof(xbeepacket::AT_RESPONSE) + 4) {
					return;
				}
				info->serial[0] = packet.value[0];
				info->serial[1] = packet.value[1];
				info->serial[2] = packet.value[2];
				info->serial[3] = packet.value[3];
				info->stage = 1;
				run_modeminfo_send_request(modem, info);
				break;

			case 1:
				if (packet.command[0] != 'S' || packet.command[1] != 'L') {
					return;
				}
				if (packet.status != xbeepacket::AT_RESPONSE_STATUS_OK) {
					std::cerr << "AT command returned error " << packet.status << '\n';
					Gtk::Main::quit();
					return;
				}
				if (length != sizeof(xbeepacket::AT_RESPONSE) + 4) {
					return;
				}
				info->serial[4] = packet.value[0];
				info->serial[5] = packet.value[1];
				info->serial[6] = packet.value[2];
				info->serial[7] = packet.value[3];
				std::cout << "Hardware Address: ";
				for (unsigned int i = 0; i < 8; ++i) {
					std::cout << Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), info->serial[i]);
				}
				std::cout << '\n';
				Gtk::Main::quit();
				break;
		}
	}

	void run_modeminfo() {
		// Create the XBee object.
		xbee modem;

		// Create a state structure.
		run_modeminfo_info info;
		info.stage = 0;

		// Attach signals.
		modem.signal_received().connect(sigc::bind(&run_modeminfo_handle_packet, &modem, &info));
		Glib::signal_timeout().connect_seconds(sigc::bind(&run_modeminfo_send_request, &modem, &info), 2);

		// Go go go!
		run_modeminfo_send_request(&modem, &info);
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
	unsigned int do_world = 0, do_sim = 0, do_quick = 0, do_log = 0, do_firmware = 0, do_test = 0, do_modeminfo = 0, do_help = 0;
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

			case 'l':
				do_log++;
				break;

			case 'f':
				do_firmware++;
				break;

			case 't':
				do_test++;
				break;

			case 'm':
				do_modeminfo++;
				break;

			default:
				do_help++;
				break;
		}

	// Check for legal combinations of options.
	if (do_help || (do_world + do_sim + do_log + do_firmware + do_test + do_modeminfo != 1)) {
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
		clocksource_timerfd time_clk(UINT64_C(1000000000) / TIMESTEPS_PER_SECOND);
		if (do_quick) {
			clocksource_quick quick_clk;
			time_clk.start();
			simulate(quick_clk, time_clk);
		} else {
			simulate(time_clk, time_clk);
		}
	} else if (do_log) {
		log_tool_launcher launcher;
		Gtk::Main::run();
	} else if (do_firmware) {
		manage_firmware();
	} else if (do_test) {
		run_tester();
	} else if (do_modeminfo) {
		run_modeminfo();
	}

	// The configuration file might recently have been dirtied but not flushed
	// yet if the user quit the application immediately after the dirtying.
	// Flush the configuration file now.
	config::force_save();

	return 0;
}

