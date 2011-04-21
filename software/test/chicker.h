#ifndef TEST_CHICKER_H
#define TEST_CHICKER_H

#include "xbee/robot.h"
#include <gtkmm.h>

/**
 * \brief A panel that lets the user manually control the kicking subsystem.
 */
class ChickerPanel : public Gtk::Table {
	public:
		/**
		 * \brief Constructs a new ChickerPanel.
		 *
		 * \param[in] robot the robot to control.
		 */
		ChickerPanel(XBeeRobot::Ptr robot);

		/**
		 * \brief Shuts down the charger.
		 */
		void scram();

		/**
		 * \brief Fires the solenoids.
		 */
		void fire();

	private:
		XBeeRobot::Ptr robot;
		Gtk::CheckButton charge;
		Gtk::Label pulse_width1_label;
		Gtk::HScale pulse_width1;
		Gtk::Label pulse_width2_label;
		Gtk::HScale pulse_width2;
		Gtk::Label pulse_offset_label;
		Gtk::HScale pulse_offset;
		Gtk::HBox fire_hbox;
		Gtk::Button kick;
		Gtk::ToggleButton autokick;

		void on_alive_changed();
		void on_charge_changed();
		void on_pulse_width_changed();
		void on_pulse_offset_changed();
		void on_kick();
		void on_autokick_changed();
};

#endif

