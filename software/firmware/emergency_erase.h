#ifndef FIRMWARE_EMERGENCY_ERASE_H
#define FIRMWARE_EMERGENCY_ERASE_H

#include "firmware/watchable_operation.h"
#include <stdint.h>

//
// An in-progress attempt to signal an emergency erase.
//
class emergency_erase : public watchable_operation, public sigc::trackable {
	public:
		//
		// Constructs an emergency erase object.
		//
		emergency_erase(xbee &modem, uint64_t bot);

		//
		// Starts the erase process.
		//
		void start();

	private:
		xbee &modem;
		uint64_t bot;
		sigc::connection packet_received_connection, timeout_connection;
		
		void report_error(const Glib::ustring &error);
		bool timeout();
		void receive(const void *, std::size_t);
};

#endif

