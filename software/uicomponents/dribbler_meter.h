#ifndef UICOMPONENTS_DRIBBLER_METER_H
#define UICOMPONENTS_DRIBBLER_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing dribbler speed for a robot.
//
class DribblerMeter : public Gtk::ProgressBar, public NonCopyable {
	public:
		//
		// Constructs an DribblerMeter with no robot.
		//
		DribblerMeter();

		//
		// Sets which robot this dribbler meter will monitor.
		//
		void set_bot(RefPtr<XBeeDriveBot> bot);

	private:
		RefPtr<XBeeDriveBot> robot;
		sigc::connection connection;
		unsigned int last_speed;

		void update();
};

#endif

