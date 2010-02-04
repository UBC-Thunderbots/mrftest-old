#ifndef UICOMPONENTS_RSSI_METER_H
#define UICOMPONENTS_RSSI_METER_H

#include "xbee/bot.h"
#include <iomanip>
#include <gtkmm.h>

//
// A meter showing an RSSI to or from a robot.
//
class rssi_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs an rssi_meter with no robot.
		//
		rssi_meter(int (radio_bot::*func)() const) : func(func), last_rssi(1) {
			set_fraction(0);
			set_text("No Data");
		}

		//
		// Sets which robot this RSSI meter will monitor.
		//
		void set_bot(radio_bot::ptr bot) {
			conn.disconnect();
			robot = bot;
			update();
			if (robot) {
				conn = robot->signal_updated().connect(sigc::mem_fun(*this, &rssi_meter::update));
			}
		}

	private:
		int (radio_bot::*func)() const;
		radio_bot::ptr robot;
		sigc::connection conn;
		int last_rssi;

		void update() {
			if (robot && robot->has_feedback()) {
				// This is a disgusting hack because Glib::RefPtr<> doesn't
				// provide operator->*.
				int rssi = (robot.operator->()->*func)();
				if (rssi != last_rssi) {
					set_fraction((rssi + 255) / 255.0);
					set_text(Glib::ustring::compose("%1dBm", rssi));
					last_rssi = rssi;
				}
			} else {
				if (last_rssi != 1) {
					set_fraction(0);
					set_text("No Data");
					last_rssi = 1;
				}
			}
		}
};

#endif

