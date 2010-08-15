#ifndef UICOMPONENTS_OUTBOUND_RSSI_METER_H
#define UICOMPONENTS_OUTBOUND_RSSI_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

/**
 * A meter showing outbound RSSI to a robot.
 */
class OutboundRSSIMeter : public Gtk::ProgressBar, public NonCopyable {
	public:
		/**
		 * Constructs an OutboundRSSIMeter with no robot.
		 */
		OutboundRSSIMeter();

		/**
		 * Sets which robot this RSSI meter will monitor.
		 *
		 * \param[in] bot the robot to monitor.
		 */
		void set_bot(XBeeDriveBot::Ptr bot);

	private:
		XBeeDriveBot::Ptr robot;
		sigc::connection connection;
		int last_rssi;

		void update();
};

#endif

