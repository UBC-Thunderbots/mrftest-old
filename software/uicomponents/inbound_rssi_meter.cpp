#include "uicomponents/inbound_rssi_meter.h"

#include <iomanip>

inbound_rssi_meter::inbound_rssi_meter() : last_rssi(-1) {
	set_fraction(0);
	set_text("No Data");
}

void inbound_rssi_meter::set_bot(xbee_drive_bot::ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &inbound_rssi_meter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_rssi = -1;
}

void inbound_rssi_meter::update() {
	int rssi = -robot->inbound_rssi();
	if (rssi != last_rssi) {
		set_fraction((rssi + 255) / 255.0);
		set_text(Glib::ustring::compose("%1dBm", rssi));
		last_rssi = rssi;
	}
}

