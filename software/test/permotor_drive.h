#ifndef TESTER_PERMOTOR_DRIVE_H
#define TESTER_PERMOTOR_DRIVE_H

#include "test/zeroable.h"
#include "xbee/robot.h"
#include <gtkmm.h>

/**
 * The superclass of all drive control panels that have four sliders independently controlling the four wheels.
 */
class TesterControlPerMotorDrive : public Gtk::Table, public Zeroable {
	public:
		/**
		 * Constructs a new TesterControlPerMotorDrive.
		 *
		 * \param[in] bot the robot to control.
		 */
		TesterControlPerMotorDrive(XBeeRobot::Ptr bot);

		void zero();

	private:
		XBeeRobot::Ptr robot;

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

