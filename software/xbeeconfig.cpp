#include "util/dprint.h"
#include "xbee/dongle.h"
#include <cctype>
#include <exception>
#include <glibmm.h>
#include <iostream>
#include <sstream>

namespace {
	void parse_channels(const Glib::ustring &str, unsigned int &channel0, unsigned int &channel1) {
		// 0B - 1A
		if (str.size() == 5 && std::isxdigit(str[0]) && std::isxdigit(str[1]) && str[2] == ':' && std::isxdigit(str[3]) && std::isxdigit(str[4])) {
			{
				std::wostringstream oss;
				oss << str.substr(0, 2);
				std::wistringstream iss(oss.str());
				iss.flags(std::wistringstream::hex);
				iss >> channel0;
			}
			{
				std::wostringstream oss;
				oss << str.substr(3);
				std::wistringstream iss(oss.str());
				iss.flags(std::wistringstream::hex);
				iss >> channel1;
			}
			if (channel0 >= 0x0B && channel0 <= 0x1A && channel1 >= 0x0B && channel1 <= 0x1A) {
				return;
			}
		}
		throw std::runtime_error("Bad channel string");
	}

	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		// Parse the command-line arguments.
		Glib::OptionContext option_context;
		option_context.set_summary("Allows getting and setting dongle parameters.");
		Glib::OptionGroup option_group("thunderbots", "Configurator Options", "Show Configurator Options");

		Glib::OptionEntry set_channels_entry;
		set_channels_entry.set_long_name("set-channels");
		set_channels_entry.set_description("Writes new channel selections to the dongle");
		set_channels_entry.set_arg_description("CH0:CH1");
		Glib::ustring set_channels_value;
		option_group.add_entry(set_channels_entry, set_channels_value);

		option_context.set_main_group(option_group);
		if (!option_context.parse(argc, argv)) {
			std::cout << option_context.get_help();
			return 1;
		}
		bool do_set_channels = false;
		unsigned int channel0, channel1;
		if (!set_channels_value.empty()) {
			do_set_channels = true;
			parse_channels(set_channels_value, channel0, channel1);
		}

		// Find the dongle.
		std::cout << "Finding dongle... " << std::flush;
		XBeeDongle dongle(true);
		std::cout << "OK\n";

		// Set channel, if ordered to do so.
		if (do_set_channels) {
			dongle.set_channels(channel0, channel1);
		}

		// Get the current parameters.
		std::cout << "Getting channels... " << std::flush;
		std::pair<unsigned int, unsigned int> channels = dongle.get_channels();
		std::cout << "OK: " << tohex(channels.first, 2) << " / " << tohex(channels.second, 2) << '\n';

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

