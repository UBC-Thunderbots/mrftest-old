#include "uicomponents/inbound_rssi_meter.h"

#include <iomanip>

InboundRSSIMeter::InboundRSSIMeter() : last_rssi(-1) {
	set_fraction(0);
	set_text("No Data");
}

void InboundRSSIMeter::set_bot(XBeeDriveBot::Ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &InboundRSSIMeter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_rssi = -1;
}

void InboundRSSIMeter::update() {
	int rssi = robot->inbound_rssi();
	if (rssi != last_rssi) {
		set_fraction((rssi + 255) / 255.0);
		set_text(Glib::ustring::compose("%1dBm", rssi));
		last_rssi = rssi;
	}
}

