#ifndef UICOMPONENTS_INBOUND_RSSI_METER_H
#define UICOMPONENTS_INBOUND_RSSI_METER_H

#include "xbee/client/drive.h"
#include <gtkmm.h>

/**
 * A meter showing inbound RSSI from a robot.
 */
class InboundRSSIMeter : public Gtk::ProgressBar, public NonCopyable {
	public:
		/**
		 * Constructs an InboundRSSIMeter with no robot.
		 */
		InboundRSSIMeter();

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

