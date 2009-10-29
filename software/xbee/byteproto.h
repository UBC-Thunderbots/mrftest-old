#ifndef XBEE_BYTEPROTO_H
#define XBEE_BYTEPROTO_H

#include "util/noncopyable.h"
#include "xbee/serial.h"
#include <cstddef>
#include <stdint.h>
#include <glibmm.h>
#include <sigc++/sigc++.h>

//
// Performs the escaping necessary to provide a categorized byte stream to and from the XBee.
//
class xbee_byte_stream : public virtual noncopyable, public virtual sigc::trackable {
	public:
		//
		// Constructs a new byte stream.
		//
		xbee_byte_stream();

		//
		// Invoked when the start-of-packet delimiter is received.
		//
		sigc::signal<void> &signal_sop_received() {
			return sig_sop_received;
		}

		//
		// Invoked when a byte is received.
		//
		sigc::signal<void, uint8_t> &signal_byte_received() {
			return sig_byte_received;
		}

		//
		// Sends a start-of-packet delimiter to the port.
		//
		void send_sop();

		//
		// Sends a byte to the port.
		//
		void send(uint8_t);

		//
		// Sends a string of bytes to the port.
		//
		void send(const void *, std::size_t);

	private:
		serial_port port;
		sigc::signal<void> sig_sop_received;
		sigc::signal<void, uint8_t> sig_byte_received;
		bool escape;
		void byte_received(uint8_t b);
};

#endif

