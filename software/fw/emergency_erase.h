#ifndef FIRMWARE_EMERGENCY_ERASE_H
#define FIRMWARE_EMERGENCY_ERASE_H

#include "fw/watchable_operation.h"
#include "xbee/client/raw.h"
#include <stdint.h>

/**
 * An in-progress attempt to signal an emergency erase.
 */
class EmergencyErase : public WatchableOperation, public sigc::trackable {
	public:
		/**
		 * Constructs an emergency erase object.
		 *
		 * \param[in] bot the robot whose SPI Flash should be wiped.
		 */
		EmergencyErase(XBeeRawBot::Ptr bot);

		/**
		 * Starts the erase process.
		 */
		void start();

	private:
		const XBeeRawBot::Ptr bot;
		sigc::connection complete_connection;

		void report_error(const Glib::ustring &error);
		void on_complete(const void *, std::size_t);
};

#endif

