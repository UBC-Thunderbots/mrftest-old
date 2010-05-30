#include "tester/feedback.h"
#include "util/xbee.h"
#include "xbee/shared/packettypes.h"

tester_feedback::tester_feedback() : Gtk::Table(6, 3, false), battery_label("Battery Voltage:"), dribbler_label("Dribbler Speed:"), out_rssi_label("Out RSSI:"), in_rssi_label("In RSSI:"), latency_label("Latency:"), success_label("Delivery Rate:"), fault_indicator_frame("Motor Faults"), fault_indicator_box(true) {
	attach(battery_label, 0, 1, 0, 1, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(dribbler_label, 0, 1, 1, 2, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(out_rssi_label, 0, 1, 2, 3, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(in_rssi_label, 0, 1, 3, 4, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(latency_label, 0, 1, 4, 5, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(success_label, 0, 1, 5, 6, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);

	attach(battery_level, 1, 2, 0, 1, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(dribbler_level, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(out_rssi_level, 1, 2, 2, 3, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(in_rssi_level, 1, 2, 3, 4, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(latency_level, 1, 2, 4, 5, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);
	attach(success_level, 1, 2, 5, 6, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);

	for (unsigned int i = 0; i < 5; ++i) {
		fault_indicator_box.pack_start(fault_indicators[i]);
	}
	fault_indicator_frame.add(fault_indicator_box);
	attach(fault_indicator_frame, 2, 3, 0, 6, Gtk::SHRINK | Gtk::FILL, Gtk::EXPAND | Gtk::FILL, 3, 0);
}

void tester_feedback::set_bot(xbee_drive_bot::ptr bot) {
	connection.disconnect();
	battery_level.set_bot(bot);
	dribbler_level.set_bot(bot);
	out_rssi_level.set_bot(bot);
	in_rssi_level.set_bot(bot);
	latency_level.set_bot(bot);
	success_level.set_bot(bot);
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &tester_feedback::update));
	}
	for (unsigned int i = 0; i < 5; ++i) {
		fault_indicators[i].set_colour(0, 0, 0);
	}
}

void tester_feedback::update() {
	for (unsigned int i = 0; i < 5; ++i) {
		if (robot->feedback().faults & (1 << i)) {
			fault_indicators[i].set_colour(1, 0, 0);
		} else {
			fault_indicators[i].set_colour(0, 1, 0);
		}
	}
}

