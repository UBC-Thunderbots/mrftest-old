#include "uicomponents/latency_meter.h"
#include <iomanip>

latency_meter::latency_meter() : last_latency(-1) {
	set_fraction(0);
	set_text("No Data");
}

void latency_meter::set_bot(xbee_drive_bot::ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &latency_meter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_latency = -1;
}

void latency_meter::update() {
	int latency = robot->latency().tv_sec * 100 + ((robot->latency().tv_nsec + 5000000L) / 10000000L);
	if (latency != last_latency) {
		set_fraction(latency / 100.0);
		set_text(Glib::ustring::compose("%1s", Glib::ustring::format(std::fixed, std::setprecision(2), latency / 100.0)));
		last_latency = latency;
	}
}

