#include "test/matrix_drive.h"

TesterControlMatrixDrive::TesterControlMatrixDrive(XBeeDriveBot::Ptr bot) : Gtk::Table(3, 2), robot(bot), drive1_label("Vx:"), drive2_label("Vy:"), drive3_label("Vt:"), drive1_scale(-10, 10, .1), drive2_scale(-10, 10, .1), drive3_scale(-20, 20, .1) {
	attach(drive1_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(drive2_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(drive3_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);

	attach(drive1_scale, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(drive2_scale, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(drive3_scale, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);

	drive1_scale.set_digits(1);
	drive1_scale.set_value(0);
	drive2_scale.set_digits(1);
	drive2_scale.set_value(0);
	drive3_scale.set_digits(1);
	drive3_scale.set_value(0);
	drive1_scale.get_adjustment()->set_page_size(0);
	drive2_scale.get_adjustment()->set_page_size(0);
	drive3_scale.get_adjustment()->set_page_size(0);
	drive1_scale.signal_value_changed().connect(sigc::mem_fun(this, &TesterControlMatrixDrive::on_change));
	drive2_scale.signal_value_changed().connect(sigc::mem_fun(this, &TesterControlMatrixDrive::on_change));
	drive3_scale.signal_value_changed().connect(sigc::mem_fun(this, &TesterControlMatrixDrive::on_change));

	on_change();
}

void TesterControlMatrixDrive::on_change() {
	static const double matrix[4][3] = {
		{ -42.5995, 27.6645, 4.3175 },
		{ -35.9169, -35.9169, 4.3175 },
		{ 35.9169, -35.9169, 4.3175 },
		{ 42.5995, 27.6645, 4.3175 }
	};
	if (robot.is()) {
		double input[3] = {
			drive1_scale.get_value(),
			drive2_scale.get_value(),
			drive3_scale.get_value()
		};
		double output[4] = { 0, 0, 0, 0 };
		for (unsigned int row = 0; row < 4; ++row) {
			for (unsigned int col = 0; col < 3; ++col) {
				output[row] += matrix[row][col] * input[col];
			}
		}
		int m1 = static_cast<int>(output[0]);
		int m2 = static_cast<int>(output[1]);
		int m3 = static_cast<int>(output[2]);
		int m4 = static_cast<int>(output[3]);
		robot->drive_controlled(m1, m2, m3, m4);
	}
}

void TesterControlMatrixDrive::zero() {
	drive1_scale.set_value(0);
	drive2_scale.set_value(0);
	drive3_scale.set_value(0);
	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win) {
		win->invalidate(true);
	}
}

