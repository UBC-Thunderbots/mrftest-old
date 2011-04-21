#ifndef TEST_DRIVE_H
#define TEST_DRIVE_H

#include "xbee/robot.h"
#include <gtkmm.h>

/**
 * \brief A panel that lets the user manually control the wheels.
 */
class DrivePanel : public Gtk::VBox {
	public:
		/**
		 * \brief Constructs a new DrivePanel.
		 *
		 * \param[in] robot the robot to control.
		 */
		DrivePanel(XBeeRobot::Ptr robot);

		/**
		 * \brief Sets all speed selectors to their zero positions.
		 */
		void zero();

		/**
		 * \brief Switches the wheels into halt mode.
		 */
		void scram();

		/**
		 * \brief Sets all four sliders to specified values.
		 *
		 * \param[in] values the values to set, in the range ±1.
		 */
		void set_values(const double (&values)[4]);

	private:
		XBeeRobot::Ptr robot;
		Gtk::ComboBoxText mode_chooser;
		Gtk::HScale controls[4];

		void on_mode_changed();
		void on_update();
};

#endif

