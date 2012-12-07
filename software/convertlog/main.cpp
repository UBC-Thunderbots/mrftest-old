#include "convertlog/v1v2/convert.h"
#include <iostream>
#include <locale>
#include <stdexcept>
#include <typeinfo>
#include <glibmm/exception.h>
#include <glibmm/optioncontext.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>

namespace {
	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		// Parse the command-line arguments.
		Glib::OptionContext option_context;
		option_context.set_summary("Runs the Thunderbots log version converter.");

		Glib::OptionGroup option_group("thunderbots", "Log Converter Options", "Show Log Converter Options");

		Glib::OptionEntry v1v2_entry;
		v1v2_entry.set_long_name("v1v2");
		v1v2_entry.set_description("Converts logs from version 1 to version 2");
		bool v1v2 = false;
		option_group.add_entry(v1v2_entry, v1v2);

		option_context.set_main_group(option_group);

		if (!option_context.parse(argc, argv)) {
			std::cerr << "Invalid command-line option(s). Use --help for help.\n";
			return 1;
		}

		// Run the appropriate jobs.
		if (v1v2) {
			ConvertLogV1V2::run();
		}

		return 0;
	}
}

int main(int argc, char **argv) {
	try {
		return main_impl(argc, argv);
	} catch (const Glib::Exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch (const std::exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}

