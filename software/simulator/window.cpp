#include "simulator/engine.h"
#include "simulator/window.h"
#include "uicomponents/visualizer.h"
#include "world/playtype.h"
#include <algorithm>
#include <functional>
#include <gtkmm.h>
#include <sigc++/sigc++.h>

//
// A combo box that allows the user to select a simulation engine.
//
class engine_chooser : public virtual Gtk::ComboBoxText {
	public:
		engine_chooser(simulator &sim) : initializing(true) {
			append_text("No Engine");
			const simulator_engine_factory::map_type &m = simulator_engine_factory::all();
			for (simulator_engine_factory::map_type::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i)
				append_text(i->first);
			simulator_engine::ptr engine = sim.get_engine();
			if (engine) {
				set_active_text(engine->get_factory().name());
			} else {
				set_active_text("No Engine");
			}
			initializing = false;
		}

		sigc::signal<void, Glib::ustring> &signal_changed() {
			return the_signal_changed;
		}

	protected:
		virtual void on_changed() {
			if (!initializing) {
				const Glib::ustring &name = get_active_text();
				the_signal_changed.emit(name);
			}
		}

	private:
		sigc::signal<void, Glib::ustring> the_signal_changed;
		bool initializing;
};



//
// Controls for selecting and managing the simulation engine.
//
class engine_controls : public virtual Gtk::VBox {
	public:
		engine_controls(simulator &sim) : sim(sim), chooser(sim), ctls(0) {
			chooser.signal_changed().connect(sigc::mem_fun(*this, &engine_controls::engine_changed));
			pack_start(chooser);
			put_custom_controls();
		}

	private:
		simulator &sim;
		engine_chooser chooser;
		Widget *ctls;

		void put_custom_controls() {
			// Remove old controls.
			if (ctls)
				remove(*ctls);

			// Get the current engine.
			simulator_engine::ptr e(sim.get_engine());

			// Get controls.
			if (e)
				ctls = e->get_ui_controls();
			else
				ctls = Gtk::manage(new Gtk::Label("No simulation engine selected."));
			if (!ctls)
				ctls = Gtk::manage(new Gtk::Label("This engine provides no controls."));
			pack_start(*ctls);

			// Show the controls.
			show_all_children();
		}

		void engine_changed(const Glib::ustring &e) {
			// Lock in the use of the new engine.
			sim.set_engine(e);

			// Add the new engine-specific controls.
			put_custom_controls();
		}
};



//
// A combobox for selecting which playtype is active.
//
class playtype_combo : public virtual Gtk::ComboBoxText {
	public:
		playtype_combo(simulator &sim) : sim(sim) {
			for (unsigned int i = 0; i < playtype::count; i++) {
				append_text(playtype::descriptions_west[i]);
			}
			set_active_text("Halt");
		}

	protected:
		virtual void on_changed() {
			const Glib::ustring &pt = get_active_text();
			for (unsigned int i = 0; i < playtype::count; i++)
				if (playtype::descriptions_west[i] == pt)
					sim.set_playtype(static_cast<playtype::playtype>(i));
		}

	private:
		simulator &sim;
};



//
// The main window.
//
class simulator_window_impl : public virtual Gtk::Window {
	public:
		simulator_window_impl(simulator &sim) : sim(sim), engine_frame("Simulation Engine"), engine_ctls(sim), playtype_frame("Play Type"), playtype_cb(sim), westteam_frame("West Team"), eastteam_frame("East Team"), visualizer_frame("Visualizer"), vis(sim.fld, sim.west_ball, sim.west_team.get_west_view(), sim.east_team.get_west_view()) {
			set_title("Thunderbots Simulator");

			engine_frame.add(engine_ctls);
			vbox.pack_start(engine_frame, false, false);

			playtype_frame.add(playtype_cb);
			vbox.pack_start(playtype_frame, false, false);

			vbox.pack_start(westteam_frame, true, true);

			vbox.pack_start(eastteam_frame, true, true);

			paned.pack1(vbox, false, false);

			visualizer_frame.add(vis);
			paned.pack2(visualizer_frame, true, false);

			add(paned);
			show_all();

			sim.signal_updated().connect(sigc::mem_fun(*this, &simulator_window_impl::on_timestep));
		}

	protected:
		virtual bool on_delete_event(GdkEventAny *) {
			Gtk::Main::quit();
			return true;
		}

	private:
		simulator &sim;

		simulator_engine::ptr engine;

		Gtk::HPaned paned;

		Gtk::VBox vbox;

		Gtk::Frame engine_frame;
		engine_controls engine_ctls;

		Gtk::Frame playtype_frame;
		playtype_combo playtype_cb;

		Gtk::Frame westteam_frame;

		Gtk::Frame eastteam_frame;

		Gtk::Frame visualizer_frame;
		visualizer vis;

		void on_timestep() {
			vis.update();
		}
};



simulator_window::simulator_window(simulator &sim) : impl(new simulator_window_impl(sim)) {
}

simulator_window::~simulator_window() {
	delete impl;
}

