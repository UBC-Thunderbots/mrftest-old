#include "tester/feedback.h"
#include "xbee/packettypes.h"
#include "xbee/util.h"
#include <iomanip>



namespace {
	void present_rssi(uint8_t rssi, Gtk::ProgressBar &bar) {
		bar.set_text(Glib::ustring::compose("−%1dBm", static_cast<unsigned int>(rssi)));
		bar.set_fraction((255 - rssi) / 255.0);
	}
}



tester_feedback::tester_feedback(xbee &modem) : Gtk::HBox(true, 5), modem(modem), current_address(0), seen_packet(false), column1(true), battery_label("Battery Voltage:"), out_rssi_label("Out RSSI:"), in_rssi_label("In RSSI:"), column2(true), column3(true), fault_label("Motor Faults:"), fault_indicator_box(true), clear_fault_button(Gtk::Stock::CLEAR) {
	column1.pack_start(battery_label);
	column1.pack_start(out_rssi_label);
	column1.pack_start(in_rssi_label);
	pack_start(column1);

	battery_level.set_text("No Data");
	out_rssi_level.set_text("No Data");
	in_rssi_level.set_text("No Data");
	column2.pack_start(battery_level);
	column2.pack_start(out_rssi_level);
	column2.pack_start(in_rssi_level);
	pack_start(column2);

	for (unsigned int i = 0; i < 5; ++i) {
		fault_indicators[i].set_colour(0, 1, 0);
		fault_indicator_box.pack_start(fault_indicators[i]);
		old_fault_counters[i] = 0;
	}
	column3.pack_start(fault_label);
	column3.pack_start(fault_indicator_box);
	column3.pack_start(clear_fault_button);
	pack_start(column3);

	modem.signal_received().connect(sigc::mem_fun(*this, &tester_feedback::packet_received));
	clear_fault_button.signal_clicked().connect(sigc::mem_fun(*this, &tester_feedback::clear_fault));
}



void tester_feedback::address_changed(uint64_t address) {
	current_address = address;
}



void tester_feedback::packet_received(const void *data, std::size_t length) {
	// Check for packet being correct length.
	if (length != sizeof(xbeepacket::FEEDBACK_DATA)) {
		return;
	}

	const xbeepacket::FEEDBACK_DATA &packet = *static_cast<const xbeepacket::FEEDBACK_DATA *>(data);

	// Check for correct API ID.
	if (packet.rxhdr.apiid != xbeepacket::RECEIVE_APIID) {
		return;
	}

	// Check for correct source address.
	if (xbeeutil::address_from_bytes(packet.rxhdr.address) != current_address) {
		return;
	}

	// Check for correct flags.
	if (!(packet.flags & xbeepacket::FEEDBACK_FLAG_RUNNING)) {
		return;
	}

	// Present the battery level.
	double volts = packet.battery_level / 1023.0 * 3.3 / 470.0 * (2200.0 + 470.0);
	battery_level.set_text(Glib::ustring::compose("%1V", Glib::ustring::format(std::fixed, std::setprecision(1), volts)));
	battery_level.set_fraction(volts / 15.0);

	// Present the signal strength readings.
	present_rssi(packet.outbound_rssi, out_rssi_level);
	present_rssi(packet.rxhdr.rssi, in_rssi_level);

	// Present motor fault indications.
	uint8_t motor_fault_counters[5];
	motor_fault_counters[0] = packet.motor_fault_counters_packed[0] & 0x0F;
	motor_fault_counters[1] = packet.motor_fault_counters_packed[0] >> 4;
	motor_fault_counters[2] = packet.motor_fault_counters_packed[1] & 0x0F;
	motor_fault_counters[3] = packet.motor_fault_counters_packed[1] >> 4;
	motor_fault_counters[4] = packet.motor_fault_counters_packed[2] & 0x0F;
	for (unsigned int i = 0; i < 5; ++i) {
		if (seen_packet) {
			if (motor_fault_counters[i] != old_fault_counters[i]) {
				fault_indicators[i].set_colour(1, 0, 0);
			}
		}
		old_fault_counters[i] = motor_fault_counters[i];
	}

	// Remember that we've seen a packet.
	seen_packet = true;
}



void tester_feedback::clear_fault() {
	for (unsigned int i = 0; i < 5; ++i) {
		fault_indicators[i].set_colour(0, 1, 0);
	}
}

