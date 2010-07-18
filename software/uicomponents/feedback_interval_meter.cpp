#include "uicomponents/feedback_interval_meter.h"
#include "util/algorithm.h"
#include "util/time.h"
#include <iomanip>

FeedbackIntervalMeter::FeedbackIntervalMeter() : last_feedback_interval(-1) {
	set_fraction(0);
	set_text("No Data");
}

void FeedbackIntervalMeter::set_bot(XBeeDriveBot::ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &FeedbackIntervalMeter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_feedback_interval = -1;
}

void FeedbackIntervalMeter::update() {
	int feedback_interval = timespec_to_millis(robot->feedback_interval()) / 10;
	if (feedback_interval != last_feedback_interval) {
		set_fraction(clamp(feedback_interval / 100.0, 0.0, 1.0));
		set_text(Glib::ustring::compose("%1s", Glib::ustring::format(std::fixed, std::setprecision(2), feedback_interval / 100.0)));
		last_feedback_interval = feedback_interval;
	}
}

