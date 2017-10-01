#include "main.h"
#include <gtkmm/main.h>
#include <iostream>
#include <locale>
#include "drive/null/dongle.h"
#include "test/null/launcher.h"
#include "util/annunciator.h"
#include "util/config.h"
#include "util/main_loop.h"

int app_main(int argc, char **argv)
{
    // Set the current locale from environment variables.
    std::locale::global(std::locale(""));

    // Parse the command-line arguments.
    Glib::OptionContext option_context;
    option_context.set_summary(u8"Allows testing robot subsystems.");

    // Load the configuration file.
    Config::load();

    // Initialize GTK.
    Gtk::Main m(argc, argv, option_context);
    if (argc != 1)
    {
        std::cout << option_context.get_help();
        return 1;
    }

    // Create the window.
    Drive::Null::Dongle dongle;
    NullTesterLauncher win(dongle);
    MainLoop::run(win);

    return 0;
}
