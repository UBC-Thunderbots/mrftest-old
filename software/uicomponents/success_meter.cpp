#include "uicomponents/success_meter.h"
#include <iomanip>

success_meter::success_meter() : last_success(-1) {
	set_fraction(0);
	set_text("No Data");
}

void success_meter::set_bot(xbee_drive_bot::ptr bot) {
	update_connection.disconnect();
	dead_connection.disconnect();
	robot = bot;
	if (robot) {
		update_connection = robot->signal_feedback.connect(sigc::mem_fun(this, &success_meter::update));
		dead_connection = robot->signal_dead.connect(sigc::mem_fun(this, &success_meter::on_bot_dead));
	}
	set_fraction(0);
	set_text("No Data");
	last_success = -1;
}

void success_meter::update() {
	int success = robot->success_rate();
	if (success != last_success) {
		set_fraction(success / 16.0);
		set_text(Glib::ustring::compose("%1/16", success));
		last_success = success;
	}
}

void success_meter::on_bot_dead() {
	set_fraction(0);
	set_text("0/64");
	last_success = 0;
}

