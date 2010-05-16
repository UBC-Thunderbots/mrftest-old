#include "configeditor/window.h"
#include "util/config.h"
#include <iostream>
#include <gtkmm.h>

namespace {
	int main_impl(int argc, char **argv) {
		Gtk::Main app(argc, argv);
		config conf;
		window win(conf);
		win.show_all();
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

