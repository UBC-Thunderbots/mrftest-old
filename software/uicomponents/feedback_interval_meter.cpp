#include "uicomponents/feedback_interval_meter.h"
#include "util/time.h"
#include <iomanip>

feedback_interval_meter::feedback_interval_meter() : last_feedback_interval(-1) {
	set_fraction(0);
	set_text("No Data");
}

void feedback_interval_meter::set_bot(xbee_drive_bot::ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &feedback_interval_meter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_feedback_interval = -1;
}

void feedback_interval_meter::update() {
	int feedback_interval = timespec_to_millis(robot->feedback_interval()) / 10;
	if (feedback_interval != last_feedback_interval) {
		set_fraction(feedback_interval / 100.0);
		set_text(Glib::ustring::compose("%1s", Glib::ustring::format(std::fixed, std::setprecision(2), feedback_interval / 100.0)));
		last_feedback_interval = feedback_interval;
	}
}

