#ifndef TESTER_MATRIX_DRIVE_H
#define TESTER_MATRIX_DRIVE_H

#include "test/zeroable.h"
#include "xbee/client/drive.h"
#include <gtkmm.h>

class TesterControlMatrixDrive : public Gtk::Table, public Zeroable {
	public:
		TesterControlMatrixDrive(XBeeDriveBot::ptr);
		void zero();

	private:
		XBeeDriveBot::ptr robot;

		Gtk::Label drive1_label;
		Gtk::Label drive2_label;
		Gtk::Label drive3_label;

		Gtk::HScale drive1_scale;
		Gtk::HScale drive2_scale;
		Gtk::HScale drive3_scale;

		void on_change();
};

#endif

