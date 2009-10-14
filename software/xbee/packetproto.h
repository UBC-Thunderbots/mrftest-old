#ifndef XBEE_PACKETPROTO_H
#define XBEE_PACKETPROTO_H

#include "util/noncopyable.h"
#include "xbee/byteproto.h"
#include <vector>
#include <cstddef>
#include <glibmm.h>
#include <sigc++/sigc++.h>

//
// Allows sending and receiving packets to the XBee.
//
class xbee_packet_stream : public virtual noncopyable, public virtual sigc::trackable {
	public:
		//
		// Constructs a new xbee_packet_stream.
		//
		xbee_packet_stream(const Glib::ustring &portname);

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

