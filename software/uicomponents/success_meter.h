#ifndef UICOMPONENTS_SUCCESS_METER_H
#define UICOMPONENTS_SUCCESS_METER_H

#include "xbee/bot.h"
#include <iomanip>
#include <gtkmm.h>

//
// A meter showing the communication success of a robot.
//
class success_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs a success_meter with no robot.
		//
		success_meter() : last_success(-1) {
			set_fraction(0);
			set_text("No Data");
		}

		//
		// Sets which robot this success meter will monitor.
		//
		void set_bot(radio_bot::ptr bot) {
			conn.disconnect();
			robot = bot;
			update();
			if (robot) {
				conn = robot->signal_updated().connect(sigc::mem_fun(this, &success_meter::update));
			}
		}

	private:
		radio_bot::ptr robot;
		sigc::connection conn;
		int last_success;

		void update() {
			if (robot && robot->has_feedback()) {
				int success = robot->success_rate();
				if (success != last_success) {
					set_fraction(success / 64.0);
					set_text(Glib::ustring::compose("%1/64", success));
					last_success = success;
				}
			} else {
				if (last_success != -1) {
					set_fraction(0);
					set_text("No Data");
					last_success = -1;
				}
			}
		}
};

#endif

