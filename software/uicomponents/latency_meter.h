#ifndef UICOMPONENTS_LATENCY_METER_H
#define UICOMPONENTS_LATENCY_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing the communication latency of a robot.
//
class latency_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs a latency_meter with no robot.
		//
		latency_meter();

		//
		// Sets which robot this latency meter will monitor.
		//
		void set_bot(xbee_drive_bot::ptr bot);

	private:
		xbee_drive_bot::ptr robot;
		sigc::connection connection;
		int last_latency;

		void update();
};

#endif

