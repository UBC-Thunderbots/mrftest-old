#include "ai/window.h"

ai_window::ai_window(ai &ai) : the_ai(ai), strategy_controls(0), rc_controls(0), vis(ai.the_world->visualizer_view()) {
	set_title("AI");

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);

	Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox);
	hbox->pack_start(*Gtk::manage(new Gtk::Label("Play type:")), Gtk::PACK_SHRINK);
	playtype_entry.set_editable(false);
	hbox->pack_start(playtype_entry, Gtk::PACK_EXPAND_WIDGET);
	vbox->pack_start(*hbox, Gtk::PACK_SHRINK);

	Gtk::Frame *robots_frame = Gtk::manage(new Gtk::Frame("Robots"));
	vbox->pack_start(*robots_frame, Gtk::PACK_EXPAND_WIDGET);

	Gtk::Frame *strategy_frame = Gtk::manage(new Gtk::Frame("Strategy"));
	strategy_chooser.append_text("<Select Strategy>");
	strategy_chooser.set_active_text("<Select Strategy>");
	for (strategy_factory::map_type::const_iterator i = strategy_factory::all().begin(), iend = strategy_factory::all().end(); i != iend; ++i) {
		strategy_chooser.append_text(i->second->name);
	}
	strategy_chooser.signal_changed().connect(sigc::mem_fun(this, &ai_window::on_strategy_changed));
	strategy_vbox.pack_start(strategy_chooser, Gtk::PACK_SHRINK);
	put_strategy_controls();
	strategy_frame->add(strategy_vbox);
	vbox->pack_start(*strategy_frame, Gtk::PACK_EXPAND_WIDGET);

	Gtk::Frame *rc_frame = Gtk::manage(new Gtk::Frame("Robot Controller"));
	rc_chooser.append_text("<Select RC>");
	rc_chooser.set_active_text("<Select RC>");
	for (robot_controller_factory::map_type::const_iterator i = robot_controller_factory::all().begin(), iend = robot_controller_factory::all().end(); i != iend; ++i) {
		rc_chooser.append_text(i->second->name);
	}
	rc_chooser.signal_changed().connect(sigc::mem_fun(this, &ai_window::on_rc_changed));
	rc_vbox.pack_start(rc_chooser, Gtk::PACK_SHRINK);
	rc_frame->add(rc_vbox);
	vbox->pack_start(*rc_frame, Gtk::PACK_EXPAND_WIDGET);

	vis_button.set_label("Visualizer");
	vis_button.signal_toggled().connect(sigc::mem_fun(this, &ai_window::on_vis_toggled));
	vbox->pack_start(vis_button, Gtk::PACK_SHRINK);

	add(*vbox);

	the_ai.the_world->signal_playtype_changed.connect(sigc::mem_fun(this, &ai_window::on_playtype_changed));
	on_playtype_changed();

	show_all();

	vis_window.set_title("AI Visualizer");
	vis_window.add(vis);
}

void ai_window::on_strategy_changed() {
	const Glib::ustring &name(strategy_chooser.get_active_text());
	strategy_factory::map_type::const_iterator i = strategy_factory::all().find(name.collate_key());
	if (i != strategy_factory::all().end()) {
		the_ai.set_strategy(i->second->create_strategy(the_ai.the_world));
	} else {
		the_ai.set_strategy(strategy::ptr());
	}
	put_strategy_controls();
}

void ai_window::on_rc_changed() {
	const Glib::ustring &name(rc_chooser.get_active_text());
	robot_controller_factory::map_type::const_iterator i = robot_controller_factory::all().find(name.collate_key());
	if (i != robot_controller_factory::all().end()) {
		the_ai.set_robot_controller_factory(i->second);
	} else {
		the_ai.set_robot_controller_factory(0);
	}
}

void ai_window::put_strategy_controls() {
	if (strategy_controls) {
		strategy_vbox.remove(*strategy_controls);
		strategy_controls = 0;
	}

	const strategy::ptr strat(the_ai.get_strategy());
	if (strat) {
		strategy_controls = strat->get_ui_controls();
		if (!strategy_controls) {
			strategy_controls = Gtk::manage(new Gtk::Label("No controls."));
		}
	} else {
		strategy_controls = Gtk::manage(new Gtk::Label("No strategy selected."));
	}

	strategy_vbox.pack_start(*strategy_controls, Gtk::PACK_EXPAND_WIDGET);
	strategy_controls->show_all();
}

void ai_window::on_playtype_changed() {
	playtype_entry.set_text(playtype::descriptions_generic[the_ai.the_world->playtype()]);
}

void ai_window::on_vis_toggled() {
	if (vis_button.get_active()) {
		vis_window.show_all();
	} else {
		vis_window.hide_all();
	}
}

