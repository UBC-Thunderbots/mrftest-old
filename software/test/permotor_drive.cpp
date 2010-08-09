#include "test/permotor_drive.h"

TesterControlPerMotorDrive::TesterControlPerMotorDrive(XBeeDriveBot::Ptr bot) : Gtk::Table(4, 2, false), robot(bot), drive1_label("Drive 1:"), drive2_label("Drive 2:"), drive3_label("Drive 3:"), drive4_label("Drive 4:"), drive1_scale(-1023, 1023, 1), drive2_scale(-1023, 1023, 1), drive3_scale(-1023, 1023, 1), drive4_scale(-1023, 1023, 1) {
	attach(drive1_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(drive2_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(drive3_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(drive4_label, 0, 1, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);

	attach(drive1_scale, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(drive2_scale, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(drive3_scale, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(drive4_scale, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);

	drive1_scale.get_adjustment()->set_page_size(0);
	drive2_scale.get_adjustment()->set_page_size(0);
	drive3_scale.get_adjustment()->set_page_size(0);
	drive4_scale.get_adjustment()->set_page_size(0);
	drive1_scale.set_value(0);
	drive2_scale.set_value(0);
	drive3_scale.set_value(0);
	drive4_scale.set_value(0);
	drive1_scale.signal_value_changed().connect(sigc::mem_fun(this, &TesterControlPerMotorDrive::on_change));
	drive2_scale.signal_value_changed().connect(sigc::mem_fun(this, &TesterControlPerMotorDrive::on_change));
	drive3_scale.signal_value_changed().connect(sigc::mem_fun(this, &TesterControlPerMotorDrive::on_change));
	drive4_scale.signal_value_changed().connect(sigc::mem_fun(this, &TesterControlPerMotorDrive::on_change));
}

void TesterControlPerMotorDrive::on_change() {
	if (robot.is()) {
		int m1 = static_cast<int16_t>(drive1_scale.get_value());
		int m2 = static_cast<int16_t>(drive2_scale.get_value());
		int m3 = static_cast<int16_t>(drive3_scale.get_value());
		int m4 = static_cast<int16_t>(drive4_scale.get_value());
		drive(m1, m2, m3, m4);
	}
}

void TesterControlPerMotorDrive::zero() {
	drive1_scale.set_value(0);
	drive2_scale.set_value(0);
	drive3_scale.set_value(0);
	drive4_scale.set_value(0);
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win) {
		win->invalidate(true);
	}
}

