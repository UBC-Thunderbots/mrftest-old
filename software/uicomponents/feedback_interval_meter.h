#ifndef UICOMPONENTS_FEEDBACK_INTERVAL_METER_H
#define UICOMPONENTS_FEEDBACK_INTERVAL_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing the communication feedback interval of a robot.
//
class feedback_interval_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs a feedback_interval_meter with no robot.
		//
		feedback_interval_meter();

		//
		// Sets which robot this feedback interval meter will monitor.
		//
		void set_bot(xbee_drive_bot::ptr bot);

	private:
		xbee_drive_bot::ptr robot;
		sigc::connection connection;
		int last_feedback_interval;

		void update();
};

#endif

