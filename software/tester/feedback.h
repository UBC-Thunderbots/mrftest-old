#ifndef TESTER_FEEDBACK_H
#define TESTER_FEEDBACK_H

#include "uicomponents/battery_meter.h"
#include "uicomponents/light.h"
#include "uicomponents/rssi_meter.h"
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
		battery_meter battery_level;
		rssi_meter out_rssi_level;
		rssi_meter in_rssi_level;

		Gtk::VBox column3;
		Gtk::Label fault_label;
		Gtk::HBox fault_indicator_box;
		light fault_indicators[5];

		void on_update();
};

#endif

