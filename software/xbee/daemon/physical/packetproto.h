#ifndef XBEE_DAEMON_PHYSICAL_PACKETPROTO_H
#define XBEE_DAEMON_PHYSICAL_PACKETPROTO_H

#include "xbee/daemon/frontend/backend.h"
#include "xbee/daemon/physical/byteproto.h"
#include <vector>
#include <cstddef>

//
// Allows sending and receiving packets to the XBee.
//
class xbee_packet_stream : public backend, public sigc::trackable {
	public:
		//
		// Constructs a new xbee_packet_stream.
		//
		xbee_packet_stream();

		//
		// Configures the serial port.
		//
		void configure_port() {
			bstream.configure_port();
		}

		//
		// Sends a packet.
		//
		void send(const iovec *iov, std::size_t iovcnt);

	private:
		xbee_byte_stream bstream;
		bool sop_seen;
		uint16_t length;
		uint8_t length_seen;
		std::vector<uint8_t> buffer;
		void on_sop();
		void on_bytes(const void *, std::size_t);
};

#endif

