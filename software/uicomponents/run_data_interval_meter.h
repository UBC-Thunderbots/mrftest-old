#ifndef UICOMPONENTS_RUN_DATA_INTERVAL_METER_H
#define UICOMPONENTS_RUN_DATA_INTERVAL_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

//
// A meter showing the communication run data interval of a robot.
//
class run_data_interval_meter : public Gtk::ProgressBar, public noncopyable {
	public:
		//
		// Constructs a run_data_interval_meter with no robot.
		//
		run_data_interval_meter();

		//
		// Sets which robot this run data interval meter will monitor.
		//
		void set_bot(xbee_drive_bot::ptr bot);

	private:
		xbee_drive_bot::ptr robot;
		sigc::connection connection;
		int last_run_data_interval;

		void update();
};

#endif

