#ifndef FIRMWARE_EMERGENCY_ERASE_H
#define FIRMWARE_EMERGENCY_ERASE_H

#include "firmware/watchable_operation.h"
#include "xbee/client/raw.h"
#include <stdint.h>

//
// An in-progress attempt to signal an emergency erase.
//
class emergency_erase : public watchable_operation, public sigc::trackable {
	public:
		//
		// Constructs an emergency erase object.
		//
		emergency_erase(xbee_raw_bot::ptr bot);

		//
		// Starts the erase process.
		//
		void start();

	private:
		const xbee_raw_bot::ptr bot;
		sigc::connection complete_connection;
		
		void report_error(const Glib::ustring &error);
		void on_complete(const void *);
};

#endif

