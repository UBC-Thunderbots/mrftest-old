#include "uicomponents/outbound_rssi_meter.h"
#include <iomanip>

OutboundRSSIMeter::OutboundRSSIMeter() : last_rssi(-1) {
	set_fraction(0);
	set_text("No Data");
}

void OutboundRSSIMeter::set_bot(XBeeDriveBot::Ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &OutboundRSSIMeter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_rssi = -1;
}

void OutboundRSSIMeter::update() {
	int rssi = robot->outbound_rssi();
	if (rssi != last_rssi) {
		set_fraction((rssi + 255) / 255.0);
		set_text(Glib::ustring::compose("%1dBm", rssi));
		last_rssi = rssi;
	}
}

