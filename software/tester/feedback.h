#ifndef TESTER_FEEDBACK_H
#define TESTER_FEEDBACK_H

#include "uicomponents/light.h"
#include "xbee/bot.h"
#include <cstddef>
#include <stdint.h>
#include <gtkmm.h>

//
// Displays feedback from the robot.
//
class tester_feedback : public Gtk::HBox {
	public:
		tester_feedback();
		void set_bot(radio_bot::ptr bot);

	private:
		radio_bot::ptr robot;

		Gtk::VBox column1;
		Gtk::Label battery_label;
		Gtk::Label out_rssi_label;
		Gtk::Label in_rssi_label;

		Gtk::VBox column2;
		Gtk::ProgressBar battery_level;
		Gtk::ProgressBar out_rssi_level;
		Gtk::ProgressBar in_rssi_level;

		Gtk::VBox column3;
		Gtk::Label fault_label;
		Gtk::HBox fault_indicator_box;
		light fault_indicators[5];

		void on_update();
};

#endif

