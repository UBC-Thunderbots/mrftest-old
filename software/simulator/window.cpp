#include "robot_controller/robot_controller.h"
#include "simulator/engine.h"
#include "simulator/team.h"
#include "simulator/window.h"
#include "uicomponents/visualizer.h"
#include "world/playtype.h"
#include <algorithm>
#include <functional>
#include <sstream>
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
// A combo box that allows the user to select a strategy.
//
class strategy_chooser : public virtual Gtk::ComboBoxText {
	public:
		strategy_chooser(simulator_team_data &team) : initializing(true) {
			append_text("No Strategy");
			const strategy_factory::map_type &m = strategy_factory::all();
			for (strategy_factory::map_type::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i)
				append_text(i->first);
			strategy::ptr strat = team.get_strategy();
			if (strat) {
				set_active_text(strat->get_factory().name());
			} else {
				set_active_text("No Strategy");
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
// The controls for managing a team's strategy.
//
class strategy_controls : public virtual Gtk::VBox {
	public:
		strategy_controls(simulator_team_data &team) : team(team), chooser(team), ctls(0) {
			chooser.signal_changed().connect(sigc::mem_fun(*this, &strategy_controls::strategy_changed));
			pack_start(chooser);
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
			pack_start(*ctls);

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
class controller_chooser : public virtual Gtk::ComboBoxText {
	public:
		controller_chooser(robot_controller_factory *current) : initializing(true) {
			append_text("No Controller");
			const robot_controller_factory::map_type &m = robot_controller_factory::all();
			for (robot_controller_factory::map_type::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i)
				append_text(i->first);
			if (current)
				set_active_text(current->name());
			else
				set_active_text("No Controller");
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
// The controls for managing a team.
//
class team_controls : public virtual Gtk::VBox {
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
			pack_start(strategy_frame, false, false);

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
class simulator_window_impl : public virtual Gtk::Window {
	public:
		simulator_window_impl(simulator &sim) : sim(sim), engine_frame("Simulation Engine"), engine_ctls(sim), playtype_frame("Play Type"), playtype_cb(sim), westteam_frame("West Team"), westteam_ctls(sim.west_team), eastteam_frame("East Team"), eastteam_ctls(sim.east_team), visualizer_frame("Visualizer"), vis(sim.fld, sim.west_ball, sim.west_team.west_view, sim.east_team.west_view) {
			set_title("Thunderbots Simulator");

			engine_frame.add(engine_ctls);
			vbox.pack_start(engine_frame, false, false);

			playtype_frame.add(playtype_cb);
			vbox.pack_start(playtype_frame, false, false);

			westteam_frame.add(westteam_ctls);
			vbox.pack_start(westteam_frame, true, true);

			eastteam_frame.add(eastteam_ctls);
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
		team_controls westteam_ctls;

		Gtk::Frame eastteam_frame;
		team_controls eastteam_ctls;

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

