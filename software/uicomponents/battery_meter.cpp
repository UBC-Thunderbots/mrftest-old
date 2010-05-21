#include "uicomponents/battery_meter.h"
#include <algorithm>
#include <iomanip>

namespace {
	const double ADC_MAX = 1023.0;
	const double VCC = 3.3;
	const double DIVIDER_UPPER = 2200.0;
	const double DIVIDER_LOWER = 470.0;
}

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
		const double volts = voltage / ADC_MAX * VCC / DIVIDER_LOWER * (DIVIDER_LOWER + DIVIDER_UPPER);
		set_fraction(std::min(1.0, std::max(0.0, (volts - 14.0) / 3.0)));
		set_text(Glib::ustring::compose("%1V", Glib::ustring::format(std::fixed, std::setprecision(2), volts)));
		last_voltage = voltage;
	}
}

