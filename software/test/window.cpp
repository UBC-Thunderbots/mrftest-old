#include "test/window.h"

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

	add(hbox);

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

