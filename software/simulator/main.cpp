#include "simulator/simulator.h"
#include "util/clocksource_timerfd.h"
#include "util/timestep.h"
#include <exception>
#include <glibmm.h>
#include <iostream>
#include <locale>

namespace {
	/**
	 * The name of the default simulation engine to pick of no engine is chosen on the command line.
	 */
	const Glib::ustring DEFAULT_ENGINE("Open Dynamics Engine Simulator");

	/**
	 * Creates an engine.
	 *
	 * \param[in] name the simulation engine name provided on the command line,
	 * or the empty string if none was chosen.
	 *
	 * \return the engine, or a null pointer if the chosen engine does not exist.
	 */
	SimulatorEngine::Ptr create_engine(const Glib::ustring &name) {
		const SimulatorEngineFactory::Map &m = SimulatorEngineFactory::all();
		SimulatorEngineFactory::Map::const_iterator i = m.find(name.collate_key());
		if (i != m.end()) {
			return i->second->create_engine();
		} else {
			std::cerr << "There is no engine named '" << name << "'. The available engines are:\n";
			for (SimulatorEngineFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
				std::cerr << i->second->name() << '\n';
			}
			return SimulatorEngine::Ptr();
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
	int main_impl(int argc, char **argv) {
		std::locale::global(std::locale(""));

		Glib::OptionContext option_context;
		option_context.set_summary("Runs the Thunderbots simulator.");
		Glib::OptionGroup option_group("thunderbots", "Simulator Options", "Show Simulator Options");
		Glib::OptionEntry engine_name_entry;
		engine_name_entry.set_long_name("engine");
		engine_name_entry.set_short_name('e');
		engine_name_entry.set_description("Chooses which engine to use rather than the default (if no valid engine provided, displays a list)");
		engine_name_entry.set_arg_description("ENGINE");
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

		Glib::RefPtr<Glib::MainLoop> main_loop = Glib::MainLoop::create();

		SimulatorEngine::Ptr engine(create_engine(engine_name));
		if (engine.is()) {
			Simulator::Simulator sim(engine);
			main_loop->run();
		}
		return 0;
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

