#ifndef UICOMPONENTS_INBOUND_RSSI_METER_H
#define UICOMPONENTS_INBOUND_RSSI_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing inbound RSSI from a robot.
//
class inbound_rssi_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs an inbound_rssi_meter with no robot.
		//
		inbound_rssi_meter();

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

