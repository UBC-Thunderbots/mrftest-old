#include "tester/direct_drive.h"

tester_control_direct_drive::tester_control_direct_drive() : Gtk::HBox(false, 10), column1(true, 0), drive1_label("Drive 1:"), drive2_label("Drive 2:"), drive3_label("Drive 3:"), drive4_label("Drive 4:"), drive5_label("Dribbler:"), column2(true, 0), drive1_scale(-1023, 1023, 1), drive2_scale(-1023, 1023, 1), drive3_scale(-1023, 1023, 1), drive4_scale(-1023, 1023, 1), drive5_scale(-1023, 1023, 1) {
	column1.pack_start(drive1_label);
	column1.pack_start(drive2_label);
	column1.pack_start(drive3_label);
	column1.pack_start(drive4_label);
	column1.pack_start(drive5_label);
	pack_start(column1);

	column2.pack_start(drive1_scale);
	column2.pack_start(drive2_scale);
	column2.pack_start(drive3_scale);
	column2.pack_start(drive4_scale);
	column2.pack_start(drive5_scale);
	pack_start(column2);
}

void tester_control_direct_drive::encode(xbeepacket::RUN_DATA &data) {
	data.flags |= xbeepacket::RUN_FLAG_DIRECT_DRIVE;
	data.drive_speeds[0] = static_cast<int16_t>(drive1_scale.get_value());
	data.drive_speeds[1] = static_cast<int16_t>(drive2_scale.get_value());
	data.drive_speeds[2] = static_cast<int16_t>(drive3_scale.get_value());
	data.drive_speeds[3] = static_cast<int16_t>(drive4_scale.get_value());
	data.dribbler_speed = static_cast<int16_t>(drive5_scale.get_value());
}

void tester_control_direct_drive::scram() {
	drive1_scale.set_value(0);
	drive2_scale.set_value(0);
	drive3_scale.set_value(0);
	drive4_scale.set_value(0);
	drive5_scale.set_value(0);
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win) {
		win->invalidate(true);
	}
}

