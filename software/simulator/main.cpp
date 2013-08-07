#include "main.h"
#include "simulator/simulator.h"
#include "util/main_loop.h"
#include "util/random.h"
#include "util/timestep.h"
#include <iostream>
#include <locale>
#include <glibmm/main.h>
#include <glibmm/optioncontext.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>

namespace {
	/**
	 * The name of the default simulation engine to pick of no engine is chosen on the command line.
	 */
	const Glib::ustring DEFAULT_ENGINE(u8"Open Dynamics Engine Simulator");

	/**
	 * Creates an engine.
	 *
	 * \param[in] name the simulation engine name provided on the command line,
	 * or the empty string if none was chosen.
	 *
	 * \return the engine, or a null pointer if the chosen engine does not exist.
	 */
	std::unique_ptr<SimulatorEngine> create_engine(const Glib::ustring &name) {
		const SimulatorEngineFactory::Map &m = SimulatorEngineFactory::all();
		SimulatorEngineFactory::Map::const_iterator i = m.find(name.collate_key());
		if (i != m.end()) {
			return i->second->create_engine();
		} else {
			std::cerr << "There is no engine named '" << name << "'. The available engines are:\n";
			for (const auto &i : m) {
				std::cerr << i.second->name() << '\n';
			}
			return std::unique_ptr<SimulatorEngine>();
		}
	}
}

/**
 * Runs the simulator.
 *
 * \param[in] argc the number of command-line arguments provided.
 *
 * \param[in] argv the command-line arguments.
 *
 * \return the application's exit code.
 */
int app_main(int argc, char **argv) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

	// Seed the PRNGs.
	Random::seed();

	Glib::OptionContext option_context;
	option_context.set_summary(u8"Runs the Thunderbots simulator.");
	Glib::OptionGroup option_group(u8"thunderbots", u8"Simulator Options", u8"Show Simulator Options");
	Glib::OptionEntry engine_name_entry;
	engine_name_entry.set_long_name(u8"engine");
	engine_name_entry.set_short_name('e');
	engine_name_entry.set_description(u8"Chooses which engine to use rather than the default (if no valid engine provided, displays a list)");
	engine_name_entry.set_arg_description(u8"ENGINE");
	Glib::ustring engine_name = DEFAULT_ENGINE;
	option_group.add_entry(engine_name_entry, engine_name);
	option_context.set_main_group(option_group);

	if (!option_context.parse(argc, argv)) {
		return 1;
	}
	if (argc != 1) {
		std::cerr << option_context.get_help();
		return 1;
	}

	std::unique_ptr<SimulatorEngine> engine(create_engine(engine_name));
	if (engine) {
		Simulator::Simulator sim(*engine.get());
		MainLoop::run();
	}
	return 0;
}

