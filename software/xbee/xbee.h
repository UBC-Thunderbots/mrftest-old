#ifndef XBEE_XBEE_H
#define XBEE_XBEE_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <cstddef>
#include <stdint.h>
#include <glibmm.h>

//
// Allows access to the XBee radio modem, via the arbiter daemon.
//
class xbee : public noncopyable {
	public:
		//
		// Connects to the arbiter daemon, launching it if necessary.
		//
		xbee();

		//
		// Enqueues a packet to be sent to the modem.
		//
		void send(const void *, std::size_t);

		//
		// Fired when a packet is received.
		//
		sigc::signal<void, const void *, std::size_t> &signal_received() {
			return sig_received;
		}

		//
		// Allocates a frame number.
		//
		uint8_t alloc_frame() {
			uint8_t cur = next_frame;
			if (!++next_frame) {
				next_frame = 1;
			}
			return cur;
		}

	private:
		const file_descriptor sock;
		sigc::signal<void, const void *, std::size_t> sig_received;
		uint8_t next_frame;

		bool on_readable(Glib::IOCondition);
};

#endif

