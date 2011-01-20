#ifndef FEEDBACK_H
#define FEEDBACK_H

#include "xbee/dongle.h"
#include "xbee/robot.h"
#include <gtkmm.h>

class TesterFeedbackPanel : public Gtk::Table {
	public:
		TesterFeedbackPanel(XBeeDongle &dongle);
		~TesterFeedbackPanel();
		void set_robot(XBeeRobot::Ptr bot);

	private:
		XBeeDongle &dongle;
		XBeeRobot::Ptr robot;
		sigc::connection alive_connection;
		Gtk::ProgressBar battery_voltage, capacitor_voltage, dribbler_temperature, break_beam_reading;
		sigc::connection battery_voltage_connection, capacitor_voltage_connection, dribbler_temperature_connection, break_beam_reading_connection;
		Gtk::CheckButton estop, ball_in_beam, ball_on_dribbler, capacitor_charged;
		sigc::connection ball_in_beam_connection, ball_on_dribbler_connection, capacitor_charged_connection;

		void on_estop_changed();
		void on_alive_changed();
		void on_battery_voltage_changed();
		void on_capacitor_voltage_changed();
		void on_dribbler_temperature_changed();
		void on_break_beam_reading_changed();
		void on_ball_in_beam_changed();
		void on_ball_on_dribbler_changed();
		void on_capacitor_charged_changed();
};

#endif

