#ifndef FIRMWARE_EMERGENCY_ERASE_H
#define FIRMWARE_EMERGENCY_ERASE_H

#include "firmware/watchable_operation.h"
#include "xbee/client/raw.h"
#include <stdint.h>

//
// An in-progress attempt to signal an emergency erase.
//
class EmergencyErase : public WatchableOperation, public sigc::trackable {
	public:
		//
		// Constructs an emergency erase object.
		//
		EmergencyErase(XBeeRawBot::ptr bot);

		//
		// Starts the erase process.
		//
		void start();

	private:
		const XBeeRawBot::ptr bot;
		sigc::connection complete_connection;
		
		void report_error(const Glib::ustring &error);
		void on_complete(const void *, std::size_t);
};

#endif

