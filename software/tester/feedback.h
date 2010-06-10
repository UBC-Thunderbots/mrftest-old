#ifndef TESTER_FEEDBACK_H
#define TESTER_FEEDBACK_H

#include "uicomponents/battery_meter.h"
#include "uicomponents/dribbler_meter.h"
#include "uicomponents/feedback_interval_meter.h"
#include "uicomponents/inbound_rssi_meter.h"
#include "uicomponents/latency_meter.h"
#include "uicomponents/light.h"
#include "uicomponents/outbound_rssi_meter.h"
#include "uicomponents/run_data_interval_meter.h"
#include "uicomponents/success_meter.h"
#include <cstddef>
#include <gtkmm.h>

//
// Displays feedback from the robot.
//
class tester_feedback : public Gtk::Table {
	public:
		tester_feedback();
		void set_bot(xbee_drive_bot::ptr bot);

	private:
		xbee_drive_bot::ptr robot;

		Gtk::Label battery_label;
		Gtk::Label dribbler_label;
		Gtk::Label out_rssi_label;
		Gtk::Label in_rssi_label;
		Gtk::Label latency_label;
		Gtk::Label feedback_interval_label;
		Gtk::Label run_data_interval_label;
		Gtk::Label success_label;

		battery_meter battery_level;
		dribbler_meter dribbler_level;
		outbound_rssi_meter out_rssi_level;
		inbound_rssi_meter in_rssi_level;
		latency_meter latency_level;
		feedback_interval_meter feedback_interval_level;
		run_data_interval_meter run_data_interval_level;
		success_meter success_level;

		Gtk::Frame fault_indicator_frame;
		Gtk::HBox fault_indicator_box;
		light fault_indicators[5];

		sigc::connection connection;

		void update();
};

#endif

