#ifndef MRFTEST_KICKER_H
#define MRFTEST_KICKER_H

#include "mrf/robot.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/table.h>
#include <gtkmm/togglebutton.h>
/**
 * \brief A panel that lets the user manually control the kicking subsystem.
 */
class KickerPanel : public Gtk::Table {
	public:
		/**
		 * \brief Constructs a new KickerPanel.
		 *
		 * \param[in] robot the robot to control.
		 */
		KickerPanel(MRFRobot &robot);

		/**
		 * \brief Shuts down the charger.
		 */
		void scram();

		/**
		 * \brief Fires the solenoids.
		 */
		void fire();

	private:
		MRFRobot &robot;
		Gtk::HBox charge_box;
		Gtk::RadioButtonGroup charge_group;
		Gtk::RadioButton discharge_button, float_button, charge_button;
		Gtk::HBox solenoid_box;
		Gtk::RadioButtonGroup solenoid_group;
		Gtk::RadioButton kicker_button, chipper_button;
		Gtk::Label pulse_width_label;
		Gtk::HScale pulse_width;
		Gtk::HBox fire_hbox;
		Gtk::Button kick;
		Gtk::ToggleButton autokick;
		Gtk::Label autokick_count_label;
		Gtk::Label autokick_count_value_label;
		unsigned int autokick_count;

		void on_alive_changed();
		void on_charge_changed();
		void on_kick();
		void on_autokick_changed();
		void on_autokick_fired();
};

#endif

