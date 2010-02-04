#ifndef UICOMPONENTS_LATENCY_METER_H
#define UICOMPONENTS_LATENCY_METER_H

#include "xbee/bot.h"
#include <iomanip>
#include <gtkmm.h>

//
// A meter showing the communication latency of a robot.
//
class latency_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs a latency_meter with no robot.
		//
		latency_meter() : last_latency(-1) {
			set_fraction(0);
			set_text("No Data");
		}

		//
		// Sets which robot this latency meter will monitor.
		//
		void set_bot(radio_bot::ptr bot) {
			conn.disconnect();
			robot = bot;
			update();
			if (robot) {
				conn = robot->signal_updated().connect(sigc::mem_fun(*this, &latency_meter::update));
			}
		}

	private:
		radio_bot::ptr robot;
		sigc::connection conn;
		int last_latency;

		void update() {
			if (robot && robot->has_feedback()) {
				int latency = robot->latency() * 100 + 0.5;
				if (latency != last_latency) {
					set_fraction(latency / 100.0);
					set_text(Glib::ustring::compose("%1s", Glib::ustring::format(std::fixed, std::setprecision(2), latency / 100.0)));
					last_latency = latency;
				}
			} else {
				if (last_latency != -1) {
					set_fraction(0);
					set_text("No Data");
					last_latency = -1;
				}
			}
		}
};

#endif

