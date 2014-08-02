#ifndef TEST_COMMON_DRIVE_H
#define TEST_COMMON_DRIVE_H

#include "drive/robot.h"
#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/scale.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/window.h>

/**
 * \brief A panel that lets the user manually control the wheels
 */
class DrivePanel : public Gtk::VBox {
	public:
		/**
		 * \brief Constructs a new DrivePanel
		 *
		 * \param[in] robot the robot to control
		 */
		DrivePanel(Drive::Robot &robot);

		/**
		 * \brief Sets all speed selectors to their zero positions
		 */
		void zero();

		/**
		 * \brief Switches the wheels into coast mode
		 */
		void coast();

		/**
		 * \brief Sets all four sliders to specified values
		 *
		 * \param[in] values the values to set, in the range ±1
		 */
		void set_values(const double(&values)[4]);

		/**
		 * \brief Retrieves the scale factors to apply to the four axes when operating with a joystick in low-sensitivity mode
		 *
		 * \param[out] scale the scale factors
		 */
		void get_low_sensitivity_scale_factors(double (&scale)[4]);

	private:
		Drive::Robot &robot;
		Gtk::ComboBoxText mode_chooser;
		Gtk::HScale controls[4];
		Gtk::CheckButton controllers_checkbox;
		Gtk::ToggleButton manual_commutation_button;

		void on_mode_changed();
		void on_update();
};

#endif

