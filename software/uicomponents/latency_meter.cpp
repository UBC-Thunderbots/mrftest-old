#include "uicomponents/latency_meter.h"
#include "util/algorithm.h"
#include "util/time.h"
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
	int latency = timespec_to_millis(robot->latency()) / 10;
	if (latency != last_latency) {
		set_fraction(clamp(latency / 100.0, 0.0, 1.0));
		set_text(Glib::ustring::compose("%1s", Glib::ustring::format(std::fixed, std::setprecision(2), latency / 100.0)));
		last_latency = latency;
	}
}

