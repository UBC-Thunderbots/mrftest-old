#ifndef UICOMPONENTS_SUCCESS_METER_H
#define UICOMPONENTS_SUCCESS_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing the communication success of a robot.
//
class success_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs a success_meter with no robot.
		//
		success_meter();

		//
		// Sets which robot this success meter will monitor.
		//
		void set_bot(xbee_drive_bot::ptr bot);

	private:
		xbee_drive_bot::ptr robot;
		sigc::connection connection;
		int last_success;

		void update();
};

#endif

