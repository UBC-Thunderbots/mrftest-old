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

/**
 * Displays feedback from the robot.
 */
class TesterFeedback : public Gtk::Table {
	public:
		/**
		 * Creates a new TesterFeedback.
		 */
		TesterFeedback();

		/**
		 * Sets which bot this feedback object will monitor.
		 *
		 * \param[in] bot the robot to monitor.
		 */
		void set_bot(XBeeDriveBot::Ptr bot);

	private:
		XBeeDriveBot::Ptr robot;

		Gtk::Label battery_label;
		Gtk::Label dribbler_label;
		Gtk::Label out_rssi_label;
		Gtk::Label in_rssi_label;
		Gtk::Label latency_label;
		Gtk::Label feedback_interval_label;
		Gtk::Label run_data_interval_label;
		Gtk::Label success_label;

		BatteryMeter battery_level;
		DribblerMeter dribbler_level;
		OutboundRSSIMeter out_rssi_level;
		InboundRSSIMeter in_rssi_level;
		LatencyMeter latency_level;
		FeedbackIntervalMeter feedback_interval_level;
		RunDataIntervalMeter run_data_interval_level;
		SuccessMeter success_level;

		Gtk::Frame fault_indicator_frame;
		Gtk::HBox fault_indicator_box;
		Light fault_indicators[5];

		sigc::connection connection;

		void update();
};

#endif

