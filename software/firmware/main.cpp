#include "firmware/window.h"
#include "util/config.h"
#include "xbee/client/lowlevel.h"
#include <gtkmm.h>
#include <iostream>

namespace {
	int main_impl(int argc, char **argv) {
		Gtk::Main m(argc, argv);
		config conf;
		if (!conf.robots().size()) {
			Gtk::MessageDialog md("There are no robots configured. Please run the configuration editor to add some.", false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			md.set_title("Thunderbots Firmware Uploader");
			md.run();
			return 0;
		}
		xbee_lowlevel modem;
		firmware_window win(modem, conf);
		Gtk::Main::run(win);
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

