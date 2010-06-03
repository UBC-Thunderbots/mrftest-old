#include "uicomponents/outbound_rssi_meter.h"
#include <iomanip>

outbound_rssi_meter::outbound_rssi_meter() : last_rssi(-1) {
	set_fraction(0);
	set_text("No Data");
}

void outbound_rssi_meter::set_bot(xbee_drive_bot::ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &outbound_rssi_meter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_rssi = -1;
}

void outbound_rssi_meter::update() {
	int rssi = robot->outbound_rssi();
	if (rssi != last_rssi) {
		set_fraction((rssi + 255) / 255.0);
		set_text(Glib::ustring::compose("%1dBm", rssi));
		last_rssi = rssi;
	}
}

