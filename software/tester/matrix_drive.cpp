#include "tester/matrix_drive.h"

tester_control_matrix_drive::tester_control_matrix_drive() : Gtk::HBox(false, 10), column1(true, 0), drive1_label("Vx:"), drive2_label("Vy:"), drive3_label("Vt:"), column2(true, 0), drive1_scale(-10, 10.1, .1), drive2_scale(-10, 10.1, .1), drive3_scale(-20, 20.1, .1) {
	column1.pack_start(drive1_label);
	column1.pack_start(drive2_label);
	column1.pack_start(drive3_label);
	pack_start(column1);

	column2.pack_start(drive1_scale);
	column2.pack_start(drive2_scale);
	column2.pack_start(drive3_scale);
	pack_start(column2);

	drive1_scale.set_digits(1);
	drive2_scale.set_digits(1);
	drive3_scale.set_digits(1);

	drive1_scale.signal_value_changed().connect(sigc::mem_fun(*this, &tester_control_matrix_drive::on_change));
	drive2_scale.signal_value_changed().connect(sigc::mem_fun(*this, &tester_control_matrix_drive::on_change));
	drive3_scale.signal_value_changed().connect(sigc::mem_fun(*this, &tester_control_matrix_drive::on_change));
}

void tester_control_matrix_drive::set_robot(radio_bot::ptr bot) {
	robot = bot;
	on_change();
}

void tester_control_matrix_drive::on_change() {
	static const double matrix[4][3] = {
		{-42.5995, 27.6645, 4.3175},
		{-35.9169, -35.9169, 4.3175},
		{35.9169, -35.9169, 4.3175},
		{42.5995, 27.6645, 4.3175}
	};
	if (robot) {
		double input[3] = {
			drive1_scale.get_value(),
			drive2_scale.get_value(),
			drive3_scale.get_value()
		};
		double output[4] = {0, 0, 0, 0};
		for (unsigned int row = 0; row < 4; ++row)
			for (unsigned int col = 0; col < 3; ++col)
				output[row] += matrix[row][col] * input[col];
		int16_t m1 = static_cast<int16_t>(output[0]);
		int16_t m2 = static_cast<int16_t>(output[1]);
		int16_t m3 = static_cast<int16_t>(output[2]);
		int16_t m4 = static_cast<int16_t>(output[3]);
		robot->drive_controlled(m1, m2, m3, m4);
	}
}

void tester_control_matrix_drive::zero() {
	drive1_scale.set_value(0);
	drive2_scale.set_value(0);
	drive3_scale.set_value(0);
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win) {
		win->invalidate(true);
	}
}

