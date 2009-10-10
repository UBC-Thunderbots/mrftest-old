#ifndef XBEE_SERIAL_H
#define XBEE_SERIAL_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <stdint.h>
#include <sigc++/sigc++.h>
#include <glibmm.h>

//
// A serial port running at 250,000 baud.
//
class serial_port : public virtual noncopyable, public virtual sigc::trackable {
	public:
		//
		// Constructs a new serial_port.
		//
		serial_port(const Glib::ustring &filename);

		//
		// Invoked when a byte arrives on the port.
		//
		sigc::signal<void, uint8_t> &signal_received() {
			return sig_received;
		}

		//
		// Sends a byte to the port.
		//
		void send(uint8_t);

	private:
		file_descriptor fd;
		sigc::signal<void, uint8_t> sig_received;

		bool on_readable(Glib::IOCondition);
};

#endif

