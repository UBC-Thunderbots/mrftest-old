#ifndef UICOMPONENTS_DRIBBLER_METER_H
#define UICOMPONENTS_DRIBBLER_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing dribbler speed for a robot.
//
class dribbler_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs an dribbler_meter with no robot.
		//
		dribbler_meter();

		//
		// Sets which robot this dribbler meter will monitor.
		//
		void set_bot(xbee_drive_bot::ptr bot);

	private:
		xbee_drive_bot::ptr robot;
		sigc::connection connection;
		unsigned int last_speed;

		void update();
};

#endif

