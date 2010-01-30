#include "simulator/window.h"
#include "uicomponents/visualizer.h"
#include <algorithm>
#include <gtkmm.h>



//
// A combobox for selecting which playtype is active.
//
class playtype_combo : public Gtk::ComboBoxText {
	public:
		playtype_combo(simulator &sim) : sim(sim) {
			for (unsigned int i = 0; i < playtype::count; i++)
				append_text(playtype::descriptions_west[i]);
			set_active_text(playtype::descriptions_west[sim.current_playtype()]);
			sim.signal_playtype_changed().connect(sigc::mem_fun(*this, &playtype_combo::playtype_changed));
		}

	protected:
		void on_changed() {
			const Glib::ustring &pt = get_active_text();
			for (unsigned int i = 0; i < playtype::count; i++)
				if (playtype::descriptions_west[i] == pt)
					sim.set_playtype(static_cast<playtype::playtype>(i));
		}

	private:
		simulator &sim;

		void playtype_changed(playtype::playtype pt) {
			set_active_text(playtype::descriptions_west[pt]);
		}
};



//
// Controls for managing the simulation as a whole.
//
class simulation_controls : public Gtk::VBox {
	public:
		simulation_controls(simulator &sim, clocksource &simclk) : simclk(simclk), run_btn("Run"), pause_btn("Pause"), run_pause_box(Gtk::BUTTONBOX_SPREAD), playtype_cb(sim) {
			run_btn.signal_clicked().connect(sigc::mem_fun(*this, &simulation_controls::run_clicked));
			pause_btn.signal_clicked().connect(sigc::mem_fun(*this, &simulation_controls::pause_clicked));
			pause_btn.set_sensitive(false);

			run_pause_box.pack_start(run_btn);
			run_pause_box.pack_start(pause_btn);

			pack_start(run_pause_box, false, false);
			pack_start(playtype_cb, false, false);
		}

	private:
		clocksource &simclk;
		Gtk::Button run_btn, pause_btn;
		Gtk::HButtonBox run_pause_box;
		playtype_combo playtype_cb;

		void run_clicked() {
			run_btn.set_sensitive(false);
			pause_btn.set_sensitive(true);
			simclk.start();
		}

		void pause_clicked() {
			run_btn.set_sensitive(true);
			pause_btn.set_sensitive(false);
			simclk.stop();
		}
};



//
// A combo box that allows the user to select a simulation engine.
//
class engine_chooser : public Gtk::ComboBoxText {
	public:
		engine_chooser(simulator &sim) : initializing(true) {
			append_text("No Engine");
			const simulator_engine_factory::map_type &m = simulator_engine_factory::all();
			for (simulator_engine_factory::map_type::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i)
				append_text(i->second->name);
			simulator_engine::ptr engine = sim.get_engine();
			if (engine) {
				set_active_text(engine->get_factory().name);
			} else {
				set_active_text("No Engine");
			}
			initializing = false;
		}

		sigc::signal<void, Glib::ustring> &signal_changed() {
			return the_signal_changed;
		}

	protected:
		void on_changed() {
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
// A combo box that allows the user to select an autoref.
//
class autoref_chooser : public Gtk::ComboBoxText {
	public:
		autoref_chooser(simulator &sim) : initializing(true) {
			append_text("No Autoref");
			const autoref_factory::map_type &m = autoref_factory::all();
			for (autoref_factory::map_type::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i)
				append_text(i->second->name);
			autoref::ptr ref = sim.get_autoref();
			if (ref) {
				set_active_text(ref->get_factory().name);
			} else {
				set_active_text("No Autoref");
			}
			initializing = false;
		}

		sigc::signal<void, Glib::ustring> &signal_changed() {
			return the_signal_changed;
		}

	protected:
		void on_changed() {
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
// Controls for selecting and managing the simulation engine and autoref.
//
class engine_controls : public Gtk::VBox {
	public:
		engine_controls(simulator &sim, clocksource &simclk) : sim(sim), simclk(simclk), eng_chooser(sim), ref_chooser(sim) {
			eng_chooser.signal_changed().connect(sigc::mem_fun(*this, &engine_controls::engine_changed));
			ref_chooser.signal_changed().connect(sigc::mem_fun(*this, &engine_controls::autoref_changed));

			put_controls();
		}

	private:
		simulator &sim;
		clocksource &simclk;
		engine_chooser eng_chooser;
		autoref_chooser ref_chooser;

		void put_controls() {
			// Remove old controls.
			children().erase(children().begin(), children().end());

			// Get the current engine.
			simulator_engine::ptr e(sim.get_engine());

			// Get engine controls.
			Widget *engine_ctls;
			if (e)
				engine_ctls = e->get_ui_controls();
			else
				engine_ctls = Gtk::manage(new Gtk::Label("No simulation engine selected."));
			if (!engine_ctls)
				engine_ctls = Gtk::manage(new Gtk::Label("This engine provides no controls."));

			// Get the current engine.
			autoref::ptr r(sim.get_autoref());

			// Get autoref controls.
			Widget *autoref_ctls;
			if (r)
				autoref_ctls = r->get_ui_controls();
			else
				autoref_ctls = Gtk::manage(new Gtk::Label("No autoref selected."));
			if (!autoref_ctls)
				autoref_ctls = Gtk::manage(new Gtk::Label("This autoref provides no controls."));

			// Add and show children.
			pack_start(eng_chooser, false, false);
			pack_start(*engine_ctls, true, true);
			pack_start(ref_chooser, false, false);
			pack_start(*autoref_ctls, true, true);
			show_all_children();
		}

		void engine_changed(const Glib::ustring &e) {
			// Lock in the use of the new engine.
			sim.set_engine(e);

			// Add the new engine-specific controls.
			put_controls();
		}

		void autoref_changed(const Glib::ustring &r) {
			// Lock in the use of the new autoref.
			sim.set_autoref(r);

			// Add the new autoref-specific controls.
			put_controls();
		}
};



//
// A combo box that allows the user to select a strategy.
//
class strategy_chooser : public Gtk::ComboBoxText {
	public:
		strategy_chooser(simulator_team_data &team) : initializing(true) {
			append_text("No Strategy");
			const strategy_factory::map_type &m = strategy_factory::all();
			for (strategy_factory::map_type::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i)
				append_text(i->second->name);
			strategy::ptr strat = team.get_strategy();
			if (strat) {
				set_active_text(strat->get_factory().name);
			} else {
				set_active_text("No Strategy");
			}
			initializing = false;
		}

		sigc::signal<void, Glib::ustring> &signal_changed() {
			return the_signal_changed;
		}

	protected:
		void on_changed() {
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
// The controls for managing a team's strategy.
//
class strategy_controls : public Gtk::VBox {
	public:
		strategy_controls(simulator_team_data &team) : team(team), chooser(team), ctls(0) {
			chooser.signal_changed().connect(sigc::mem_fun(*this, &strategy_controls::strategy_changed));
			pack_start(chooser, false, false);
			put_custom_controls();
		}

	private:
		simulator_team_data &team;
		strategy_chooser chooser;
		Widget *ctls;

		void put_custom_controls() {
			// Remove old controls.
			if (ctls)
				remove(*ctls);

			// Get the current strategy.
			strategy::ptr s(team.get_strategy());

			// Get controls.
			if (s)
				ctls = s->get_ui_controls();
			else
				ctls = Gtk::manage(new Gtk::Label("No strategy selected."));
			if (!ctls)
				ctls = Gtk::manage(new Gtk::Label("This strategy provides no controls."));
			pack_start(*ctls, true, true);

			// Show the controls.
			show_all_children();
		}

		void strategy_changed(const Glib::ustring &s) {
			// Lock in the use of the new strategy.
			team.set_strategy(s);

			// Add the new strategy-specific controls.
			put_custom_controls();
		}
};



//
// A combo box that allows the user to select a controller.
//
class controller_chooser : public Gtk::ComboBoxText {
	public:
		controller_chooser(robot_controller_factory *current) : initializing(true) {
			append_text("No Controller");
			const robot_controller_factory::map_type &m = robot_controller_factory::all();
			for (robot_controller_factory::map_type::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i)
				append_text(i->second->name);
			if (current)
				set_active_text(current->name);
			else
				set_active_text("No Controller");
			initializing = false;
		}

		sigc::signal<void, Glib::ustring> &signal_changed() {
			return the_signal_changed;
		}

	protected:
		void on_changed() {
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
// The controls for managing a team.
//
class team_controls : public Gtk::VBox {
	public:
		team_controls(simulator_team_data &team_data) : team_data(team_data), players_frame("Players"), players_list(1), players_button_box(Gtk::BUTTONBOX_SPREAD), add_player_button(Gtk::Stock::ADD), del_player_button(Gtk::Stock::DELETE), strategy_frame("Strategy"), strategy_ctls(team_data), rc_frame("Controller"), rc_chooser(team_data.get_controller_type()) {
			pack_start(*Gtk::manage(new Gtk::Label(team_data.is_yellow() ? "Yellow" : "Blue")), false, false);

			players_list.set_headers_visible(false);
			update_model();
			players_list_scroll.add(players_list);
			players_list_scroll.set_shadow_type(Gtk::SHADOW_IN);
			players_box.pack_start(players_list_scroll, true, true);
			add_player_button.signal_clicked().connect(sigc::mem_fun(*this, &team_controls::add_player));
			players_button_box.pack_start(add_player_button);
			del_player_button.signal_clicked().connect(sigc::mem_fun(*this, &team_controls::del_player));
			players_button_box.pack_start(del_player_button);
			players_box.pack_start(players_button_box, false, false);
			players_frame.add(players_box);
			pack_start(players_frame, true, true);

			strategy_frame.add(strategy_ctls);
			pack_start(strategy_frame, true, true);

			rc_frame.add(rc_chooser);
			rc_chooser.signal_changed().connect(sigc::mem_fun(*this, &team_controls::controller_changed));
			pack_start(rc_frame, false, false);

			team_data.west_view->signal_robot_added().connect(sigc::mem_fun(*this, &team_controls::player_added));
			team_data.west_view->signal_robot_removed().connect(sigc::mem_fun(*this, &team_controls::player_removed));
		}

	private:
		void add_player() {
			team_data.add_player();
		}

		void player_added() {
			players_list.append_text(Glib::ustring::compose("%1", team_data.west_view->size() - 1));
		}

		void del_player() {
			const Gtk::ListViewText::SelectionList &sel = players_list.get_selected();
			if (sel.size() == 1) {
				team_data.remove_player(sel[0]);
				if (!team_data.impls.empty()) {
					Gtk::TreePath path;
					path.push_back(std::min(static_cast<std::vector<player_impl::ptr>::size_type>(sel[0]), team_data.impls.size() - 1));
					players_list.get_selection()->select(path);
				}
			}
		}

		void player_removed(unsigned int, robot::ptr) {
			update_model();
		}

		void update_model() {
			players_list.clear_items();
			for (unsigned int i = 0; i < team_data.west_view->size(); i++)
				players_list.append_text(Glib::ustring::compose("%1", i));
		}

		void controller_changed(const Glib::ustring &c) {
			team_data.set_controller_type(c);
		}

		simulator_team_data &team_data;

		Gtk::Frame players_frame;
		Gtk::VBox players_box;
		Gtk::ScrolledWindow players_list_scroll;
		Gtk::ListViewText players_list;
		Gtk::HButtonBox players_button_box;
		Gtk::Button add_player_button, del_player_button;

		Gtk::Frame strategy_frame;
		strategy_controls strategy_ctls;

		Gtk::Frame rc_frame;
		controller_chooser rc_chooser;
};



//
// The main window.
//
class simulator_window_impl : public Gtk::Window {
	public:
		simulator_window_impl(simulator &sim, clocksource &simclk, clocksource &uiclk) : sim(sim), sim_ctls(sim, simclk), engine_ctls(sim, simclk), westteam_ctls(sim.west_team), eastteam_ctls(sim.east_team), vis(sim.fld, sim.west_ball, sim.west_team.west_view, sim.east_team.west_view, uiclk) {
			set_title("Thunderbots Simulator");

			vbox.pack_start(sim_ctls, false, false);

			notebook.append_page(engine_ctls, "Engine");
			notebook.append_page(westteam_ctls, "West Team");
			notebook.append_page(eastteam_ctls, "East Team");
			vbox.pack_start(notebook, true, true);

			paned.pack1(vbox, false, false);

			paned.pack2(vis, true, false);

			add(paned);
			show_all();
		}

	protected:
		bool on_delete_event(GdkEventAny *) {
			Gtk::Main::quit();
			return true;
		}

	private:
		simulator &sim;

		simulator_engine::ptr engine;

		Gtk::HPaned paned;

		Gtk::VBox vbox;

		simulation_controls sim_ctls;

		Gtk::Notebook notebook;
		engine_controls engine_ctls;
		team_controls westteam_ctls;
		team_controls eastteam_ctls;

		visualizer vis;
};



simulator_window::simulator_window(simulator &sim, clocksource &simclk, clocksource &uiclk) : impl(new simulator_window_impl(sim, simclk, uiclk)) {
}

simulator_window::~simulator_window() {
	delete impl;
}

void simulator_window::show_fps(unsigned int fps) {
	impl->set_title(Glib::ustring::compose("Thunderbots Simulator - %1FPS", fps));
}

