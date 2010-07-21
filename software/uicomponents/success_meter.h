#ifndef UICOMPONENTS_SUCCESS_METER_H
#define UICOMPONENTS_SUCCESS_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing the communication success of a robot.
//
class SuccessMeter : public Gtk::ProgressBar, public NonCopyable {
	public:
		//
		// Constructs a SuccessMeter with no robot.
		//
		SuccessMeter();

		//
		// Sets which robot this success meter will monitor.
		//
		void set_bot(XBeeDriveBot::ptr bot);

	private:
		XBeeDriveBot::ptr robot;
		sigc::connection update_connection, dead_connection;
		int last_success;

		void update();
		void on_bot_dead();
};

#endif

