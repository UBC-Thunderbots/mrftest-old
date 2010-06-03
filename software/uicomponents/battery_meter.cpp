#include "uicomponents/battery_meter.h"
#include <algorithm>
#include <iomanip>
#include <limits>

battery_meter::battery_meter() : last_voltage(std::numeric_limits<unsigned int>::max()) {
	set_fraction(0);
	set_text("No Data");
}

void battery_meter::set_bot(xbee_drive_bot::ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &battery_meter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_voltage = std::numeric_limits<unsigned int>::max();
}

void battery_meter::update() {
	unsigned int voltage = robot->battery_voltage();
	if (voltage != last_voltage) {
		set_fraction(std::min(1.0, std::max(0.0, (voltage - 12000) / 5000.0)));
		set_text(Glib::ustring::compose("%1V", Glib::ustring::format(std::fixed, std::setprecision(2), voltage / 1000.0)));
		last_voltage = voltage;
	}
}

