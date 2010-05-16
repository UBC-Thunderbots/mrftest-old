#include "uicomponents/battery_meter.h"
#include <algorithm>
#include <iomanip>

battery_meter::battery_meter() : last_voltage(-1) {
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
	last_voltage = -1;
}

void battery_meter::update() {
	int voltage = robot->feedback().battery_level;
	if (voltage != last_voltage) {
		set_fraction(std::min(1.0, std::max(0.0, (voltage / 100.0 - 14.0) / 3.0)));
		set_text(Glib::ustring::compose("%1V", Glib::ustring::format(std::fixed, std::setprecision(2), voltage / 100.0)));
		last_voltage = voltage;
	}
}

