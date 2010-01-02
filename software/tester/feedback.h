#ifndef TESTER_FEEDBACK_H
#define TESTER_FEEDBACK_H

#include "uicomponents/light.h"
#include "xbee/xbee.h"
#include <cstddef>
#include <stdint.h>
#include <gtkmm.h>

//
// Displays feedback from the robot.
//
class tester_feedback : public Gtk::HBox {
	public:
		tester_feedback(xbee &modem);
		void address_changed(uint64_t address);

	private:
		xbee &modem;
		uint64_t current_address;
		uint8_t old_fault_counters[5];
		bool seen_packet;

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
		Gtk::Button clear_fault_button;

		void packet_received(const void *data, std::size_t length);
		void clear_fault();
};

#endif

