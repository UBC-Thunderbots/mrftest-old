#include "main.h"
#include "log/launcher.h"
#include <locale>
#include <gtkmm/main.h>

int app_main(int argc, char **argv) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

	// Parse the command-line arguments.
	Gtk::Main app(argc, argv);

	// Show the tool launcher window.
	LogLauncher win;
	Gtk::Main::run(win);

	return 0;
}

