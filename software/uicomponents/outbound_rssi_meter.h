#ifndef UICOMPONENTS_OUTBOUND_RSSI_METER_H
#define UICOMPONENTS_OUTBOUND_RSSI_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing outbound RSSI to a robot.
//
class outbound_rssi_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs an outbound_rssi_meter with no robot.
		//
		outbound_rssi_meter();

		//
		// Sets which robot this RSSI meter will monitor.
		//
		void set_bot(xbee_drive_bot::ptr bot);

	private:
		xbee_drive_bot::ptr robot;
		sigc::connection connection;
		int last_rssi;

		void update();
};

#endif

