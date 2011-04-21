#include "test/window.h"
#include "util/joystick.h"
#include <cmath>
#include <functional>

TesterWindow::TesterWindow(XBeeDongle &dongle, XBeeRobot::Ptr robot) : robot(robot), feedback_frame("Feedback"), feedback_panel(dongle, robot), drive_frame("Drive"), drive_panel(robot), dribble_button("Dribble"), chicker_frame("Chicker"), chicker_panel(robot), params_frame("Parameters"), params_panel(robot) {
	set_title(Glib::ustring::compose("Tester (%1)", robot->index));

	feedback_frame.add(feedback_panel);
	vbox1.pack_start(feedback_frame, Gtk::PACK_SHRINK);

	drive_frame.add(drive_panel);
	vbox1.pack_start(drive_frame, Gtk::PACK_SHRINK);

	dribble_button.signal_toggled().connect(sigc::mem_fun(this, &TesterWindow::on_dribble_toggled));
	vbox1.pack_start(dribble_button, Gtk::PACK_SHRINK);

	hbox.pack_start(vbox1, Gtk::PACK_EXPAND_WIDGET);

	chicker_frame.add(chicker_panel);
	vbox2.pack_start(chicker_frame, Gtk::PACK_SHRINK);

	params_frame.add(params_panel);
	vbox2.pack_start(params_frame, Gtk::PACK_SHRINK);

	hbox.pack_start(vbox2, Gtk::PACK_EXPAND_WIDGET);

	outer_vbox.pack_start(hbox, Gtk::PACK_SHRINK);

	joystick_chooser.append_text("<No Joystick>");
	joystick_chooser.set_active(0);
	for (auto i = Joystick::all().begin(), iend = Joystick::all().end(); i != iend; ++i) {
		joystick_chooser.append_text(Glib::ustring::compose("%1 (%2)", (*i)->name, Glib::filename_to_utf8((*i)->node)));
	}
	joystick_chooser.signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_chooser_changed));
	outer_vbox.pack_start(joystick_chooser, Gtk::PACK_SHRINK);

	add(outer_vbox);

	Gtk::Main::signal_key_snooper().connect(sigc::mem_fun(this, &TesterWindow::key_snoop));

	show_all();
}

int TesterWindow::key_snoop(Widget *, GdkEventKey *event) {
	if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_Z || event->keyval == GDK_z)) {
		// Z letter scrams the system.
		drive_panel.scram();
		dribble_button.set_active(false);
		chicker_panel.scram();
	} else if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_0)) {
		// Zero digit sets all controls to zero but does not scram things.
		drive_panel.zero();
		dribble_button.set_active(false);
	}
	return 0;
}

void TesterWindow::on_dribble_toggled() {
	robot->dribble(dribble_button.get_active());
}

#warning Joystick axis and button numbers should be configurable
void TesterWindow::on_joystick_chooser_changed() {
	std::for_each(joystick_signal_connections.begin(), joystick_signal_connections.end(), std::mem_fun_ref(&sigc::connection::disconnect));
	joystick_signal_connections.clear();
	if (joystick_chooser.get_active_row_number() > 0) {
		Joystick::Ptr stick = Joystick::all()[joystick_chooser.get_active_row_number() - 1];
		joystick_signal_connections.push_back(stick->axes()[0].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_drive_axis_changed)));
		joystick_signal_connections.push_back(stick->axes()[1].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_drive_axis_changed)));
		joystick_signal_connections.push_back(stick->axes()[3].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_drive_axis_changed)));
		joystick_signal_connections.push_back(stick->axes()[4].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_drive_axis_changed)));
		joystick_signal_connections.push_back(stick->buttons()[0].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_dribble_changed)));
		joystick_signal_connections.push_back(stick->buttons()[2].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_kick_changed)));
		joystick_signal_connections.push_back(stick->buttons()[1].signal_changed().connect(sigc::mem_fun(this, &TesterWindow::on_joystick_scram_changed)));
		on_joystick_drive_axis_changed();
	}
}

void TesterWindow::on_joystick_drive_axis_changed() {
	Joystick::Ptr stick = Joystick::all()[joystick_chooser.get_active_row_number() - 1];
	double drive_axes[4];
	drive_axes[0] = -stick->axes()[4];
	drive_axes[1] = stick->axes()[3];
	drive_axes[2] = stick->axes()[0];
	drive_axes[3] = -stick->axes()[1];
	for (unsigned int i = 0; i < G_N_ELEMENTS(drive_axes); ++i) {
		drive_axes[i] = std::pow(drive_axes[i], 3);
	}
	drive_panel.set_values(drive_axes);
}

void TesterWindow::on_joystick_dribble_changed() {
	Joystick::Ptr stick = Joystick::all()[joystick_chooser.get_active_row_number() - 1];
	if (stick->buttons()[0]) {
		dribble_button.set_active(!dribble_button.get_active());
	}
}

void TesterWindow::on_joystick_kick_changed() {
	Joystick::Ptr stick = Joystick::all()[joystick_chooser.get_active_row_number() - 1];
	if (stick->buttons()[2]) {
		chicker_panel.fire();
	}
}

void TesterWindow::on_joystick_scram_changed() {
	Joystick::Ptr stick = Joystick::all()[joystick_chooser.get_active_row_number() - 1];
	if (stick->buttons()[1]) {
		drive_panel.scram();
		dribble_button.set_active(false);
		chicker_panel.scram();
	}
}

