#include "configeditor/window.h"
#include "util/config.h"
#include <clocale>
#include <gtkmm.h>
#include <iostream>

namespace {
	int main_impl(int argc, char **argv) {
		std::setlocale(LC_ALL, "");
		Glib::OptionContext option_context;
		Gtk::Main app(argc, argv, option_context);
		if (argc != 1) {
			std::cout << option_context.get_help();
			return 1;
		}
		Config conf;
		Window win(conf);
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

