#ifndef XBEEDAEMON_PACKETPROTO_H
#define XBEEDAEMON_PACKETPROTO_H

#include "xbeedaemon/byteproto.h"
#include <vector>

//
// Allows sending and receiving packets to the XBee.
//
class xbee_packet_stream : public noncopyable, public sigc::trackable {
	public:
		//
		// Constructs a new xbee_packet_stream.
		//
		xbee_packet_stream();

		//
		// Invoked when a packet is received.
		//
		sigc::signal<void, const std::vector<uint8_t> &> signal_received() {
			return sig_received;
		}

		//
		// Sends a packet.
		//
		void send(const void *, std::size_t);

		//
		// Notifies that the underlying serial_port is readable.
		//
		void readable() {
			bstream.readable();
		}

		//
		// Returns the file_descriptor of the underlying serial_port.
		//
		const file_descriptor &fd() const {
			return bstream.fd();
		}

	private:
		xbee_byte_stream bstream;
		bool sop_seen;
		uint16_t length;
		uint8_t length_seen;
		std::vector<uint8_t> buffer;
		sigc::signal<void, const std::vector<uint8_t> &> sig_received;
		void on_sop();
		void on_byte(uint8_t);
};

#endif

