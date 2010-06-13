#include "firmware/claim.h"
#include "firmware/emergency_erase.h"
#include "firmware/fpga.h"
#include "firmware/pic.h"
#include "firmware/watchable_pair.h"
#include "firmware/window.h"
#include "util/config.h"
#include "util/ihex.h"
#include "util/noncopyable.h"
#include "xbee/client/lowlevel.h"
#include "xbee/client/raw.h"
#include <clocale>
#include <gtkmm.h>
#include <iostream>

namespace {
	void on_error_cli(const Glib::ustring &message) {
		std::cout << '\n' << message << '\n';
		Gtk::Main::quit();
	}

	void on_feedback_cli(double fraction, const watchable_operation &op) {
		std::cout << '\r' << op.get_status() << ": " << static_cast<unsigned int>(fraction * 100.0) << "%             " << std::flush;
	}

	void on_finished_cli() {
		std::cout << "\rDone                                                \n";
		Gtk::Main::quit();
	}

	void run_operation_cli(watchable_operation &op) {
		op.signal_error.connect(&on_error_cli);
		op.signal_progress.connect(sigc::bind(&on_feedback_cli, sigc::ref(op)));
		op.signal_finished.connect(&on_finished_cli);
		Glib::signal_idle().connect_once(sigc::mem_fun(op, &watchable_operation::start));
		Gtk::Main::run();
	}

	int main_impl(int argc, char **argv) {
		std::setlocale(LC_ALL, "");

		Glib::OptionContext option_context;
		option_context.set_summary("Runs the firmware uploader.");

		Glib::OptionGroup option_group("thunderbots", "Firmware Uploader Options", "Show Firmware Uploader Options");

		Glib::OptionEntry robot_entry;
		robot_entry.set_long_name("robot");
		robot_entry.set_short_name('r');
		robot_entry.set_description("Selects which robot should be upgraded.");
		Glib::ustring robot;
		option_group.add_entry(robot_entry, robot);

		Glib::OptionEntry filename_entry;
		filename_entry.set_long_name("file");
		filename_entry.set_short_name('f');
		filename_entry.set_description("Selects which file should be uploaded.");
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
		config conf;
		if (!conf.robots().size()) {
			Gtk::MessageDialog md("There are no robots configured. Please run the configuration editor to add some.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			md.set_title("Thunderbots Firmware Uploader");
			md.run();
			return 0;
		}
		xbee_lowlevel modem;
		if (fpga || pic || emergency_erase) {
			if (!conf.robots().contains_name(robot)) {
				std::cout << "There is no robot named '" << robot << "'.\n";
				return 1;
			}
			const config::robot_info &botinfo(conf.robots().find(robot));
			const xbee_raw_bot::ptr bot(xbee_raw_bot::create(botinfo.address, modem));
			claim cl(bot);
			if (fpga) {
				intel_hex ihex;
				ihex.add_section(0, 16 * 1024 * 1024 / 8);
				ihex.load(filename);
				fpga_upload up(bot, ihex);
				watchable_pair p(cl, up, 0.01);
				run_operation_cli(p);
			} else if (pic) {
				intel_hex ihex;
				ihex.add_section(0x0, 0x800);
				ihex.add_section(0x800, 0x3FFF - 0x800 + 1);
				ihex.add_section(0x300000, 16);
				ihex.load(filename);
				pic_upload up(bot, ihex);
				watchable_pair p(cl, up, 0.01);
				run_operation_cli(p);
			} else if (emergency_erase) {
				::emergency_erase ee(bot);
				watchable_pair p(cl, ee, 0.5);
				run_operation_cli(p);
			}
			return 0;
		} else {
			firmware_window win(modem, conf, robot, filename);
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

