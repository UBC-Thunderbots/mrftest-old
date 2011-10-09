#ifndef TEST_KICKER_H
#define TEST_KICKER_H

#include "xbee/robot.h"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
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
		KickerPanel(XBeeRobot::Ptr robot);

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

