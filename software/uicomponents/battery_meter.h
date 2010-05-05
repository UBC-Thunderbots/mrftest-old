#ifndef UICOMPONENTS_BATTERY_METER_H
#define UICOMPONENTS_BATTERY_METER_H

#include "xbee/bot.h"
#include <iomanip>
#include <gtkmm.h>

//
// A meter showing the battery level of a robot.
//
class battery_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs a battery_meter with no robot.
		//
		battery_meter() : last_voltage(-1) {
			set_fraction(0);
			set_text("No Data");
		}

		//
		// Sets which robot this battery meter will monitor.
		//
		void set_bot(radio_bot::ptr bot) {
			conn.disconnect();
			robot = bot;
			update();
			if (robot) {
				conn = robot->signal_updated().connect(sigc::mem_fun(this, &battery_meter::update));
			}
		}

	private:
		radio_bot::ptr robot;
		sigc::connection conn;
		int last_voltage;

		void update() {
			if (robot && robot->has_feedback()) {
				int voltage = robot->battery_voltage() * 100 + 0.5;
				if (voltage != last_voltage) {
					set_fraction(voltage / 100.0 / 17.0);
					set_text(Glib::ustring::compose("%1V", Glib::ustring::format(std::fixed, std::setprecision(2), voltage / 100.0)));
					last_voltage = voltage;
				}
			} else {
				if (last_voltage != -1) {
					set_fraction(0);
					set_text("No Data");
					last_voltage = -1;
				}
			}
		}
};

#endif

