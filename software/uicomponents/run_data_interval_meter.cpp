#include "uicomponents/run_data_interval_meter.h"
#include <iomanip>

run_data_interval_meter::run_data_interval_meter() : last_run_data_interval(-1) {
	set_fraction(0);
	set_text("No Data");
}

void run_data_interval_meter::set_bot(xbee_drive_bot::ptr bot) {
	connection.disconnect();
	robot = bot;
	if (robot) {
		connection = robot->signal_feedback.connect(sigc::mem_fun(this, &run_data_interval_meter::update));
	}
	set_fraction(0);
	set_text("No Data");
	last_run_data_interval = -1;
}

void run_data_interval_meter::update() {
	int run_data_interval = robot->run_data_interval().tv_sec * 100 + ((robot->run_data_interval().tv_nsec + 5000000L) / 10000000L);
	if (run_data_interval != last_run_data_interval) {
		set_fraction(run_data_interval / 100.0);
		set_text(Glib::ustring::compose("%1s", Glib::ustring::format(std::fixed, std::setprecision(2), run_data_interval / 100.0)));
		last_run_data_interval = run_data_interval;
	}
}

