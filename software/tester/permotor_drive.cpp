#include "tester/permotor_drive.h"

tester_control_permotor_drive::tester_control_permotor_drive() : Gtk::HBox(false, 10), column1(true, 0), drive1_label("Drive 1:"), drive2_label("Drive 2:"), drive3_label("Drive 3:"), drive4_label("Drive 4:"), column2(true, 0), drive1_scale(-1023, 1023, 1), drive2_scale(-1023, 1023, 1), drive3_scale(-1023, 1023, 1), drive4_scale(-1023, 1023, 1) {
	column1.pack_start(drive1_label);
	column1.pack_start(drive2_label);
	column1.pack_start(drive3_label);
	column1.pack_start(drive4_label);
	pack_start(column1);

	column2.pack_start(drive1_scale);
	column2.pack_start(drive2_scale);
	column2.pack_start(drive3_scale);
	column2.pack_start(drive4_scale);
	pack_start(column2);

	drive1_scale.get_adjustment()->set_page_size(0);
	drive2_scale.get_adjustment()->set_page_size(0);
	drive3_scale.get_adjustment()->set_page_size(0);
	drive4_scale.get_adjustment()->set_page_size(0);
	drive1_scale.signal_value_changed().connect(sigc::mem_fun(*this, &tester_control_permotor_drive::on_change));
	drive2_scale.signal_value_changed().connect(sigc::mem_fun(*this, &tester_control_permotor_drive::on_change));
	drive3_scale.signal_value_changed().connect(sigc::mem_fun(*this, &tester_control_permotor_drive::on_change));
	drive4_scale.signal_value_changed().connect(sigc::mem_fun(*this, &tester_control_permotor_drive::on_change));
}

void tester_control_permotor_drive::set_robot(radio_bot::ptr bot) {
	robot = bot;
	on_change();
}

void tester_control_permotor_drive::on_change() {
	if (robot) {
		int16_t m1 = static_cast<int16_t>(drive1_scale.get_value());
		int16_t m2 = static_cast<int16_t>(drive2_scale.get_value());
		int16_t m3 = static_cast<int16_t>(drive3_scale.get_value());
		int16_t m4 = static_cast<int16_t>(drive4_scale.get_value());
		drive(m1, m2, m3, m4);
	}
}

void tester_control_permotor_drive::zero() {
	drive1_scale.set_value(0);
	drive2_scale.set_value(0);
	drive3_scale.set_value(0);
	drive4_scale.set_value(0);
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win) {
		win->invalidate(true);
	}
}

