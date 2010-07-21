#ifndef UICOMPONENTS_FEEDBACK_INTERVAL_METER_H
#define UICOMPONENTS_FEEDBACK_INTERVAL_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing the communication feedback interval of a robot.
//
class FeedbackIntervalMeter : public Gtk::ProgressBar, public NonCopyable {
	public:
		//
		// Constructs a FeedbackIntervalMeter with no robot.
		//
		FeedbackIntervalMeter();

		//
		// Sets which robot this feedback interval meter will monitor.
		//
		void set_bot(XBeeDriveBot::ptr bot);

	private:
		XBeeDriveBot::ptr robot;
		sigc::connection connection;
		int last_feedback_interval;

		void update();
};

#endif

