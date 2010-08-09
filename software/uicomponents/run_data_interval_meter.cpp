#include "uicomponents/run_data_interval_meter.h"
#include "util/algorithm.h"
#include "util/time.h"
#include <iomanip>

RunDataIntervalMeter::RunDataIntervalMeter() : last_run_data_interval(-1) {
	set_fraction(0);
	set_text("No Data");
}

void RunDataIntervalMeter::set_bot(XBeeDriveBot::Ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot.is()) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &RunDataIntervalMeter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_run_data_interval = -1;
}

void RunDataIntervalMeter::update() {
	int run_data_interval = timespec_to_millis(robot->run_data_interval()) / 10;
	if (run_data_interval != last_run_data_interval) {
		set_fraction(clamp(run_data_interval / 100.0, 0.0, 1.0));
		set_text(Glib::ustring::compose("%1s", Glib::ustring::format(std::fixed, std::setprecision(2), run_data_interval / 100.0)));
		last_run_data_interval = run_data_interval;
	}
}

