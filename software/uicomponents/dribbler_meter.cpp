#include "uicomponents/dribbler_meter.h"
#include "util/algorithm.h"
#include <iomanip>
#include <limits>

dribbler_meter::dribbler_meter() : last_speed(std::numeric_limits<unsigned int>::max()) {
	set_fraction(0);
	set_text("No Data");
}

void dribbler_meter::set_bot(xbee_drive_bot::ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &dribbler_meter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_speed = std::numeric_limits<unsigned int>::max();
}

void dribbler_meter::update() {
	unsigned int speed = robot->dribbler_speed();
	if (speed != last_speed) {
		set_fraction(clamp(speed / 40000.0, 0.0, 1.0));
		set_text(Glib::ustring::compose("%1rpm", speed));
		last_speed = speed;
	}
}

