#ifndef XBEE_SERIAL_H
#define XBEE_SERIAL_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <cstddef>
#include <stdint.h>
#include <sigc++/sigc++.h>

//
// A serial port running at 250,000 baud.
//
class serial_port : public noncopyable, public sigc::trackable {
	public:
		//
		// Constructs a new serial_port.
		//
		serial_port();

		//
		// Invoked when a byte arrives on the port.
		//
		sigc::signal<void, const void *, std::size_t> &signal_received() {
			return sig_received;
		}

		//
		// Sends a byte to the port.
		//
		void send(uint8_t);

		//
		// Sends a string of bytes to the port.
		//
		void send(const void *, std::size_t);

		//
		// Notifies that the port is ready to read.
		//
		void readable();

		//
		// Returns the underlying file descriptor.
		//
		const file_descriptor &fd() const {
			return port;
		}

	private:
		const file_descriptor port;
		sigc::signal<void, const void *, std::size_t> sig_received;
};

#endif

