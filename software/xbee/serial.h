#ifndef XBEE_SERIAL_H
#define XBEE_SERIAL_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <queue>
#include <cstddef>
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
		serial_port();

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

		//
		// Sends a string of bytes to the port.
		//
		void send(const void *, std::size_t);

		//
		// Locks the port for exclusive access.
		//
		void lock(const sigc::slot<void> &on_acquire, bool batch);

		//
		// Releases the lock on the port.
		//
		void unlock();

	private:
		const file_descriptor sock;
		const file_descriptor port;
		sigc::signal<void, uint8_t> sig_received;
		std::queue<sigc::slot<void> > lock_callbacks;
		bool locked;

		bool on_port_readable(Glib::IOCondition);
		bool on_sock_readable(Glib::IOCondition);
};

#endif

