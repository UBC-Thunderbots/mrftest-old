#ifndef TESTER_MATRIX_DRIVE_H
#define TESTER_MATRIX_DRIVE_H

#include "test/zeroable.h"
#include "xbee/robot.h"
#include <gtkmm.h>

/**
 * Allows driving the robot by specifying the <var>x</var> and <var>y</var> components of the robot's linear velocity and its angular velocity.
 */
class TesterControlMatrixDrive : public Gtk::Table, public Zeroable {
	public:
		/**
		 * Constructs a new TesterControlMatrixDrive.
		 *
		 * \param[in] bot the robot to control.
		 */
		TesterControlMatrixDrive(XBeeRobot::Ptr bot);

		void zero();

	private:
		XBeeRobot::Ptr robot;

		Gtk::Label drive1_label;
		Gtk::Label drive2_label;
		Gtk::Label drive3_label;

		Gtk::HScale drive1_scale;
		Gtk::HScale drive2_scale;
		Gtk::HScale drive3_scale;

		void on_change();
};

#endif

