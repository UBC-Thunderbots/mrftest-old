#include "tester/feedback.h"
#include "xbee/packettypes.h"
#include "xbee/util.h"
#include <iomanip>



namespace {
	void present_rssi(int rssi, Gtk::ProgressBar &bar) {
		bar.set_text(Glib::ustring::compose("%1dBm", rssi));
		bar.set_fraction((rssi + 255) / 255.0);
	}
}



tester_feedback::tester_feedback() : Gtk::HBox(true, 5), column1(true), battery_label("Battery Voltage:"), out_rssi_label("Out RSSI:"), in_rssi_label("In RSSI:"), column2(true), column3(true), fault_label("Motor Faults:"), fault_indicator_box(true) {
	column1.pack_start(battery_label);
	column1.pack_start(out_rssi_label);
	column1.pack_start(in_rssi_label);
	pack_start(column1);

	column2.pack_start(battery_level);
	column2.pack_start(out_rssi_level);
	column2.pack_start(in_rssi_level);
	pack_start(column2);

	for (unsigned int i = 0; i < 5; ++i) {
		fault_indicator_box.pack_start(fault_indicators[i]);
	}
	column3.pack_start(fault_label);
	column3.pack_start(fault_indicator_box);
	pack_start(column3);

	on_update();
}



void tester_feedback::set_bot(radio_bot::ptr bot) {
	robot = bot;
	if (bot) {
		bot->signal_updated().connect(sigc::mem_fun(*this, &tester_feedback::on_update));
	} else {
		on_update();
	}
}



void tester_feedback::on_update() {
	if (robot && robot->has_feedback()) {
		battery_level.set_text(Glib::ustring::compose("%1V", robot->battery_voltage()));
		battery_level.set_fraction(robot->battery_voltage() / 17.0);
		present_rssi(robot->out_rssi(), out_rssi_level);
		present_rssi(robot->in_rssi(), in_rssi_level);
		for (unsigned int i = 0; i < 4; ++i) {
			if (robot->drive_fault(i)) {
				fault_indicators[i].set_colour(1, 0, 0);
			} else {
				fault_indicators[i].set_colour(0, 1, 0);
			}
		}
		if (robot->dribbler_fault()) {
			fault_indicators[4].set_colour(1, 0, 0);
		} else {
			fault_indicators[4].set_colour(0, 1, 0);
		}
	} else {
		battery_level.set_text("No Data");
		battery_level.set_fraction(0);
		out_rssi_level.set_text("No Data");
		out_rssi_level.set_fraction(0);
		in_rssi_level.set_text("No Data");
		in_rssi_level.set_fraction(0);
		for (unsigned int i = 0; i < 5; ++i) {
			fault_indicators[i].set_colour(0, 0, 0);
		}
	}
}

