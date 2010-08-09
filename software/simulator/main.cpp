#include "simulator/main_window.h"
#include "simulator/simulator.h"
#include "util/clocksource_timerfd.h"
#include "util/timestep.h"
#include "xbee/daemon/frontend/daemon.h"
#include <clocale>
#include <exception>
#include <gtkmm.h>
#include <iostream>

namespace {
	const Glib::ustring DEFAULT_ENGINE("Open Dynamics Engine Simulator");

	SimulatorEngine::Ptr create_engine(const Glib::ustring &name) {
		const SimulatorEngineFactory::map_type &m = SimulatorEngineFactory::all();
		if (name.size()) {
			SimulatorEngineFactory::map_type::const_iterator i = m.find(name.collate_key());
			if (i != m.end()) {
				return i->second->create_engine();
			} else {
				std::cerr << "There is no engine named '" << name << "'. The available engines are:\n";
				for (SimulatorEngineFactory::map_type::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
					std::cerr << i->second->name << '\n';
				}
				return SimulatorEngine::Ptr();
			}
		} else {
			Gtk::Dialog dlg("Thunderbots Simulator", true);
			dlg.get_vbox()->pack_start(*Gtk::manage(new Gtk::Label("Select an engine:")), Gtk::PACK_SHRINK);
			Gtk::ComboBoxText combo;
			for (SimulatorEngineFactory::map_type::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
				combo.append_text(i->second->name);
			}
			combo.set_active_text(DEFAULT_ENGINE);
			dlg.get_vbox()->pack_start(combo, Gtk::PACK_SHRINK);
			dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
			dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			dlg.set_default_response(Gtk::RESPONSE_OK);
			dlg.show_all();
			const int resp = dlg.run();
			if (resp != Gtk::RESPONSE_OK) {
				return SimulatorEngine::Ptr();
			}
			return create_engine(combo.get_active_text());
		}
	}

	int main_impl(int argc, char **argv) {
		std::setlocale(LC_ALL, "");
		Glib::OptionContext option_context;
		option_context.set_summary("Runs the Thunderbots simulator.");

		Glib::OptionGroup option_group("thunderbots", "Simulator Options", "Show Simulator Options");
		Glib::OptionEntry engine_name_entry;
		engine_name_entry.set_long_name("engine");
		engine_name_entry.set_short_name('e');
		engine_name_entry.set_description("Preselects an engine rather than querying for one at startup");
		engine_name_entry.set_arg_description("ENGINE");
		Glib::ustring engine_name;
		option_group.add_entry(engine_name_entry, engine_name);
		option_context.set_main_group(option_group);

		Gtk::Main app(argc, argv, option_context);
		if (argc != 1) {
			std::cout << option_context.get_help();
			return 1;
		}
		Config conf;
		SimulatorEngine::Ptr engine(create_engine(engine_name));
		if (engine.is()) {
			TimerFDClockSource clk((UINT64_C(1000000000) + TIMESTEPS_PER_SECOND / 2) / TIMESTEPS_PER_SECOND);
			Simulator sim(conf, engine, clk);
			XBeeDaemon d(sim);
			MainWindow win(sim);
			clk.start();
			Gtk::Main::run(win);
		}
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

