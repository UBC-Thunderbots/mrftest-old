#ifndef UICOMPONENTS_BATTERY_METER_H
#define UICOMPONENTS_BATTERY_METER_H

#include "util/noncopyable.h"
#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing the battery level of a robot.
//
class battery_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs a battery_meter with no robot.
		//
		battery_meter();

		//
		// Sets which robot this battery meter will monitor.
		//
		void set_bot(xbee_drive_bot::ptr bot);

	private:
		xbee_drive_bot::ptr robot;
		sigc::connection connection;
		unsigned int last_voltage;

		void update();
};

#endif

