#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "xbee/dongle.h"
#include "xbee/robot.h"
#include <gtkmm.h>

/**
 * \brief A panel that lets the user view various pieces of information about a robot.
 */
class TesterFeedbackPanel : public Gtk::Table {
	public:
		/**
		 * \brief Constructs a new TesterFeedbackPanel.
		 *
		 * \param[in] dongle the radio dongle over which to communicate.
		 *
		 * \param[in] robot the robot whose information should be displayed.
		 */
		TesterFeedbackPanel(XBeeDongle &dongle, XBeeRobot::Ptr robot);

	private:
		XBeeDongle &dongle;
		XBeeRobot::Ptr robot;
		Gtk::Label battery_voltage_label, capacitor_voltage_label, dribbler_temperature_label, break_beam_reading_label;
		Gtk::ProgressBar battery_voltage, capacitor_voltage, dribbler_temperature, break_beam_reading;
		Gtk::HBox cb_hbox1, cb_hbox2;
		Gtk::CheckButton alive, estop, ball_in_beam, ball_on_dribbler, capacitor_charged;

		void on_has_feedback_changed();
		void on_battery_voltage_changed();
		void on_capacitor_voltage_changed();
		void on_dribbler_temperature_changed();
		void on_break_beam_reading_changed();
		void on_alive_changed();
		void on_estop_changed();
		void on_ball_in_beam_changed();
		void on_ball_on_dribbler_changed();
		void on_capacitor_charged_changed();
};

#endif

