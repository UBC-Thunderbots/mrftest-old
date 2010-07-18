#ifndef UICOMPONENTS_LATENCY_METER_H
#define UICOMPONENTS_LATENCY_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing the communication latency of a robot.
//
class LatencyMeter : public Gtk::ProgressBar, public NonCopyable {
	public:
		//
		// Constructs a LatencyMeter with no robot.
		//
		LatencyMeter();

		//
		// Sets which robot this latency meter will monitor.
		//
		void set_bot(XBeeDriveBot::ptr bot);

	private:
		XBeeDriveBot::ptr robot;
		sigc::connection connection;
		int last_latency;

		void update();
};

#endif

