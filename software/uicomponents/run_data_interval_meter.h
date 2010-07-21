#ifndef UICOMPONENTS_RUN_DATA_INTERVAL_METER_H
#define UICOMPONENTS_RUN_DATA_INTERVAL_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing the communication run data interval of a robot.
//
class RunDataIntervalMeter : public Gtk::ProgressBar, public NonCopyable {
	public:
		//
		// Constructs a RunDataIntervalMeter with no robot.
		//
		RunDataIntervalMeter();

		//
		// Sets which robot this run data interval meter will monitor.
		//
		void set_bot(XBeeDriveBot::Ptr bot);

	private:
		XBeeDriveBot::Ptr robot;
		sigc::connection connection;
		int last_run_data_interval;

		void update();
};

#endif

