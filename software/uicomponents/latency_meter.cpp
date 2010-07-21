#include "uicomponents/latency_meter.h"
#include "util/algorithm.h"
#include "util/time.h"
#include <iomanip>

LatencyMeter::LatencyMeter() : last_latency(-1) {
	set_fraction(0);
	set_text("No Data");
}

void LatencyMeter::set_bot(XBeeDriveBot::Ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &LatencyMeter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_latency = -1;
}

void LatencyMeter::update() {
	int latency = timespec_to_millis(robot->latency()) / 10;
	if (latency != last_latency) {
		set_fraction(clamp(latency / 100.0, 0.0, 1.0));
		set_text(Glib::ustring::compose("%1s", Glib::ustring::format(std::fixed, std::setprecision(2), latency / 100.0)));
		last_latency = latency;
	}
}

