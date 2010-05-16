#include "ai/window.h"
#include "ai/world/world.h"
#include "util/clocksource_timerfd.h"
#include "util/config.h"
#include "util/timestep.h"
#include "xbee/client/drive.h"
#include "xbee/client/lowlevel.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <gtkmm.h>

namespace {
	int main_impl(int argc, char **argv) {
		Glib::OptionContext option_context;
		option_context.set_summary("Runs the Thunderbots control process.");

		Glib::OptionGroup option_group("thunderbots", "Control Process Options", "Show Control Process Options");

		Glib::OptionEntry all_entry;
		all_entry.set_long_name("all");
		all_entry.set_short_name('a');
		all_entry.set_description("Places all robots in the configuration file onto the friendly team");
		bool all = false;
		option_group.add_entry(all_entry, all);

		Glib::OptionEntry blue_entry;
		blue_entry.set_long_name("blue");
		blue_entry.set_short_name('b');
		blue_entry.set_description("Places all robots with blue lids onto the friendly team");
		bool blue = false;
		option_group.add_entry(blue_entry, blue);

		Glib::OptionEntry yellow_entry;
		yellow_entry.set_long_name("yellow");
		yellow_entry.set_short_name('y');
		yellow_entry.set_description("Places all robots with yellow lids onto the friendly team");
		bool yellow = false;
		option_group.add_entry(yellow_entry, yellow);

		Glib::OptionEntry custom_entry;
		custom_entry.set_long_name("custom");
		custom_entry.set_short_name('c');
		custom_entry.set_description("Allows a custom configuration of who is on the friendly team");
		bool custom = false;
		option_group.add_entry(custom_entry, custom);

		option_context.set_main_group(option_group);

		Gtk::Main app(argc, argv, option_context);

		if (!all && !blue && !yellow && !custom) {
			std::cerr << option_context.get_help();
			return 1;
		}

		const bool options[4] = { all, blue, yellow, custom };
		if (std::count(options, options + sizeof(options) / sizeof(*options), true) > 1) {
			std::cerr << option_context.get_help();
			return 1;
		}

		config conf;
		if (all) {
			// Keep all robots in config.
		} else if (blue || yellow) {
			// Keep only blue or yellow robots.
			std::vector<uint64_t> to_remove;
			for (unsigned int i = 0; i < conf.robots().size(); ++i) {
				if (conf.robots()[i].yellow != yellow) {
					to_remove.push_back(conf.robots()[i].address);
				}
			}
			std::for_each(to_remove.begin(), to_remove.end(), sigc::mem_fun(conf.robots(), &config::robot_set::remove));
		} else if (custom) {
#warning WRITE CODE HERE
			std::cerr << "Custom team building is not supported yet.\n";
			return 1;
		} else {
			std::cerr << "WTF happened?\n";
			return 1;
		}

		xbee_lowlevel modem;

		std::vector<xbee_drive_bot::ptr> xbee_bots;
		for (unsigned int i = 0; i < conf.robots().size(); ++i) {
			xbee_bots.push_back(xbee_drive_bot::create(conf.robots()[i].address, modem));
		}

		world::ptr the_world(world::create(conf, xbee_bots));

		clocksource_timerfd clk(UINT64_C(1000000000) / TIMESTEPS_PER_SECOND);

		ai the_ai(the_world, clk);

		ai_window win(the_ai);

		clk.start();

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

