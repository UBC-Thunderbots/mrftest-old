#ifndef TESTER_PERMOTOR_DRIVE_H
#define TESTER_PERMOTOR_DRIVE_H

#include "test/zeroable.h"
#include "xbee/client/drive.h"
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
		TesterControlPerMotorDrive(XBeeDriveBot::Ptr bot);

		void zero();

		/**
		 * Drives the robot.
		 *
		 * \param[in] m1 the level of the front-left wheel.
		 *
		 * \param[in] m2 the level of the back-left wheel.
		 *
		 * \param[in] m3 the level of the back-right wheel.
		 *
		 * \param[in] m4 the level of the front-right wheel.
		 */
		virtual void drive(int m1, int m2, int m3, int m4) = 0;

	protected:
		XBeeDriveBot::Ptr robot;

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

