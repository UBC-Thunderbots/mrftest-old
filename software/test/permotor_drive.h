#ifndef TESTER_PERMOTOR_DRIVE_H
#define TESTER_PERMOTOR_DRIVE_H

#include "test/zeroable.h"
#include "xbee/client/drive.h"
#include <gtkmm.h>

class TesterControlPerMotorDrive : public Gtk::Table, public Zeroable {
	public:
		TesterControlPerMotorDrive(RefPtr<XBeeDriveBot>);
		void zero();
		virtual void drive(int m1, int m2, int m3, int m4) = 0;

	protected:
		RefPtr<XBeeDriveBot> robot;

	private:
		Gtk::Label drive1_label;
		Gtk::Label drive2_label;
		Gtk::Label drive3_label;
		Gtk::Label drive4_label;

		Gtk::HScale drive1_scale;
		Gtk::HScale drive2_scale;
		Gtk::HScale drive3_scale;
		Gtk::HScale drive4_scale;

		void on_change();
};

#endif

