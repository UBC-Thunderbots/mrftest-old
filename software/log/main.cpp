#include "main.h"
#include <gtkmm/main.h>
#include <locale>
#include "log/launcher.h"
#include "util/main_loop.h"

int app_main(int argc, char **argv)
{
    // Set the current locale from environment variables.
    std::locale::global(std::locale(""));

    // Parse the command-line arguments.
    Gtk::Main app(argc, argv);

    // Show the tool launcher window.
    LogLauncher win;
    MainLoop::run(win);

    return 0;
}
