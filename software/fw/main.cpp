#include "fw/claim.h"
#include "fw/emergency_erase.h"
#include "fw/fpga.h"
#include "fw/ihex.h"
#include "fw/pic.h"
#include "fw/watchable_pair.h"
#include "fw/window.h"
#include "util/config.h"
#include "util/noncopyable.h"
#include "xbee/client/lowlevel.h"
#include "xbee/client/raw.h"
#include <gtkmm.h>
#include <iostream>
#include <locale>

namespace {
	void on_error_cli(const Glib::ustring &message) {
		std::cout << '\n' << message << '\n';
		Gtk::Main::quit();
	}

	void on_feedback_cli(double fraction, const WatchableOperation &op) {
		std::cout << '\r' << op.get_status() << ": " << static_cast<unsigned int>(fraction * 100.0) << "%             " << std::flush;
	}

	void on_finished_cli() {
		std::cout << "\rDone                                                \n";
		Gtk::Main::quit();
	}

	void run_operation_cli(WatchableOperation &op) {
		op.signal_error.connect(&on_error_cli);
		op.signal_progress.connect(sigc::bind(&on_feedback_cli, sigc::ref(op)));
		op.signal_finished.connect(&on_finished_cli);
		Glib::signal_idle().connect_once(sigc::mem_fun(op, &WatchableOperation::start));
		Gtk::Main::run();
	}

	int main_impl(int argc, char **argv) {
		std::locale::global(std::locale(""));

		Glib::OptionContext option_context;
		option_context.set_summary("Runs the firmware uploader.");

		Glib::OptionGroup option_group("thunderbots", "Firmware Uploader Options", "Show Firmware Uploader Options");

		Glib::OptionEntry robot_entry;
		robot_entry.set_long_name("robot");
		robot_entry.set_short_name('r');
		robot_entry.set_description("Selects which robot should be upgraded.");
		robot_entry.set_arg_description("PATTERN-INDEX");
		int robot = -1;
		option_group.add_entry(robot_entry, robot);

		Glib::OptionEntry filename_entry;
		filename_entry.set_long_name("file");
		filename_entry.set_short_name('f');
		filename_entry.set_description("Selects which file should be uploaded.");
		filename_entry.set_arg_description("FILE");
		std::string filename;
		option_group.add_entry_filename(filename_entry, filename);

		Glib::OptionEntry fpga_entry;
		fpga_entry.set_long_name("fpga");
		fpga_entry.set_description("Uploads the file to the FPGA.");
		bool fpga = false;
		option_group.add_entry(fpga_entry, fpga);

		Glib::OptionEntry pic_entry;
		pic_entry.set_long_name("pic");
		pic_entry.set_description("Uploads the file to the PIC.");
		bool pic = false;
		option_group.add_entry(pic_entry, pic);

		Glib::OptionEntry emergency_erase_entry;
		emergency_erase_entry.set_long_name("emergency-erase");
		emergency_erase_entry.set_description("Erases the FPGA's Flash storage.");
		bool emergency_erase = false;
		option_group.add_entry(emergency_erase_entry, emergency_erase);

		option_context.set_main_group(option_group);

		Gtk::Main m(argc, argv, option_context);
		if (argc != 1 || ((fpga ? 1 : 0) + (pic ? 1 : 0) + (emergency_erase ? 1 : 0) > 1)) {
			std::cout << option_context.get_help();
			return 1;
		}
		Config conf;
		if (!conf.robots().size()) {
			Gtk::MessageDialog md("There are no robots configured. Please run the configuration editor to add some.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			md.set_title("Thunderbots Firmware Uploader");
			md.run();
			return 0;
		}
		XBeeLowLevel modem;
		if (fpga || pic || emergency_erase) {
			if (!conf.robots().contains_pattern(robot)) {
				std::cout << "There is no robot named '" << robot << "'.\n";
				return 1;
			}
			const Config::RobotInfo &botinfo(conf.robots().find(robot));
			const XBeeRawBot::Ptr bot(XBeeRawBot::create(botinfo.address, modem));
			Claim cl(bot);
			if (fpga) {
				IntelHex ihex;
				ihex.add_section(0, 16 * 1024 * 1024 / 8);
				ihex.load(filename);
				FPGAUpload up(bot, ihex);
				WatchablePair p(cl, up, 0.01);
				run_operation_cli(p);
			} else if (pic) {
				IntelHex ihex;
				ihex.add_section(0x0, 0x800);
				ihex.add_section(0x800, 0x3FFF - 0x800 + 1);
				ihex.add_section(0x300000, 16);
				ihex.load(filename);
				PICUpload up(bot, ihex);
				WatchablePair p(cl, up, 0.01);
				run_operation_cli(p);
			} else if (emergency_erase) {
				EmergencyErase ee(bot);
				WatchablePair p(cl, ee, 0.5);
				run_operation_cli(p);
			}
			return 0;
		} else {
			FirmwareWindow win(modem, conf, robot, filename);
			Gtk::Main::run(win);
		}
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

