#include "uicomponents/success_meter.h"
#include <iomanip>

SuccessMeter::SuccessMeter() : last_success(-1) {
	set_fraction(0);
	set_text("No Data");
}

void SuccessMeter::set_bot(XBeeDriveBot::Ptr bot) {
	update_connection.disconnect();
	dead_connection.disconnect();
	robot = bot;
	if (robot.is()) {
		update_connection = robot->signal_feedback.connect(sigc::mem_fun(this, &SuccessMeter::update));
		dead_connection = robot->signal_dead.connect(sigc::mem_fun(this, &SuccessMeter::on_bot_dead));
	}
	set_fraction(0);
	set_text("No Data");
	last_success = -1;
}

void SuccessMeter::update() {
	int success = robot->success_rate();
	if (success != last_success) {
		set_fraction(success / 16.0);
		set_text(Glib::ustring::compose("%1/16", success));
		last_success = success;
	}
}

void SuccessMeter::on_bot_dead() {
	set_fraction(0);
	set_text("0/64");
	last_success = 0;
}

