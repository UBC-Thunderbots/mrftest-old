#ifndef XBEE_DAEMON_PHYSICAL_SERIAL_H
#define XBEE_DAEMON_PHYSICAL_SERIAL_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <cstddef>
#include <glibmm.h>
#include <cstddef>
#include <stdint.h>
#include <sigc++/sigc++.h>
#include <sys/uio.h>

//
// A serial port running at 250,000 baud.
//
class serial_port : public noncopyable, public sigc::trackable {
	public:
		//
		// Constructs a new serial_port. Opens but does not configure the port.
		//
		serial_port();

		//
		// Configures the port.
		//
		void configure_port();

		//
		// Invoked when a byte arrives on the port.
		//
		sigc::signal<void, const void *, std::size_t> &signal_received() {
			return sig_received;
		}

		//
		// Sends a string of bytes to the port.
		//
		void send(iovec *iov, std::size_t iovcnt);

	private:
		const file_descriptor port;
		sigc::signal<void, const void *, std::size_t> sig_received;
		bool on_readable(Glib::IOCondition);
};

#endif

