#ifndef TESTER_MATRIX_DRIVE_H
#define TESTER_MATRIX_DRIVE_H

#include "tester/zeroable.h"
#include "xbee/client/drive.h"
#include <gtkmm.h>

class tester_control_matrix_drive : public Gtk::Table, public zeroable {
	public:
		tester_control_matrix_drive(xbee_drive_bot::ptr);
		void zero();

	private:
		xbee_drive_bot::ptr robot;

		Gtk::Label drive1_label;
		Gtk::Label drive2_label;
		Gtk::Label drive3_label;

		Gtk::HScale drive1_scale;
		Gtk::HScale drive2_scale;
		Gtk::HScale drive3_scale;

		void on_change();
};

#endif

