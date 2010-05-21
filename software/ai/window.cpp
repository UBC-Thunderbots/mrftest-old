#include "ai/window.h"

ai_window::ai_window(ai &ai) : the_ai(ai), strategy_controls(0), rc_controls(0), vis(ai.the_world->visualizer_view()) {
	set_title("AI");

	Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox);

	Gtk::Frame *basic_frame = Gtk::manage(new Gtk::Frame("Basics"));
	Gtk::Table *basic_table = Gtk::manage(new Gtk::Table(3, 3));
	basic_table->attach(*Gtk::manage(new Gtk::Label("Play type:")), 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	playtype_entry.set_editable(false);
	basic_table->attach(playtype_entry, 1, 3, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	basic_table->attach(*Gtk::manage(new Gtk::Label("Defending:")), 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	end_entry.set_editable(false);
	basic_table->attach(end_entry, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	Gtk::Button *flip_ends_button = Gtk::manage(new Gtk::Button("X"));
	flip_ends_button->signal_clicked().connect(sigc::mem_fun(this, &ai_window::on_flip_ends_clicked));
	basic_table->attach(*flip_ends_button, 2, 3, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	basic_table->attach(*Gtk::manage(new Gtk::Label("Refbox Colour:")), 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	refbox_colour_entry.set_editable(false);
	basic_table->attach(refbox_colour_entry, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	Gtk::Button *flip_refbox_colour_button = Gtk::manage(new Gtk::Button("X"));
	flip_refbox_colour_button->signal_clicked().connect(sigc::mem_fun(this, &ai_window::on_flip_refbox_colour_clicked));
	basic_table->attach(*flip_refbox_colour_button, 2, 3, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	basic_frame->add(*basic_table);
	vbox->pack_start(*basic_frame, Gtk::PACK_SHRINK);

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
	the_ai.the_world->signal_flipped_ends.connect(sigc::mem_fun(this, &ai_window::on_flipped_ends));
	the_ai.the_world->signal_flipped_refbox_colour.connect(sigc::mem_fun(this, &ai_window::on_flipped_refbox_colour));
	on_playtype_changed();
	on_flipped_ends();
	on_flipped_refbox_colour();

	show_all();

	vis_window.set_title("AI Visualizer");
	vis_window.add(vis);
	vis_window.signal_delete_event().connect(sigc::hide(sigc::bind_return(sigc::bind(sigc::mem_fun(vis_button, &Gtk::ToggleButton::set_active), false), false)));
}

void ai_window::on_flip_ends_clicked() {
	the_ai.the_world->flip_ends();
}

void ai_window::on_flip_refbox_colour_clicked() {
	the_ai.the_world->flip_refbox_colour();
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

void ai_window::on_flipped_ends() {
	end_entry.set_text(the_ai.the_world->east() ? "East" : "West");
}

void ai_window::on_flipped_refbox_colour() {
	refbox_colour_entry.set_text(the_ai.the_world->refbox_yellow() ? "Yellow" : "Blue");
}

