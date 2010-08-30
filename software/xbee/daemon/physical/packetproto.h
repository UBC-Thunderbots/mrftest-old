#ifndef XBEE_DAEMON_PHYSICAL_PACKETPROTO_H
#define XBEE_DAEMON_PHYSICAL_PACKETPROTO_H

#include "xbee/daemon/frontend/backend.h"
#include "xbee/daemon/physical/byteproto.h"
#include <vector>
#include <cstddef>

/**
 * Allows sending and receiving packets to the XBee.
 */
class XBeePacketStream : public BackEnd, public sigc::trackable {
	public:
		/**
		 * Constructs a new XBeePacketStream.
		 */
		XBeePacketStream();

		/**
		 * Configures the serial port.
		 */
		void configure_port() {
			bstream.configure_port();
		}

		/**
		 * Sends a packet.
		 *
		 * \param[in] iov a pointer to an array of iovecs to gather to find the bytes to send.
		 *
		 * \param[in] iovcnt the number of iovecs in the \p iov array.
		 */
		void send(const iovec *iov, std::size_t iovcnt);

	private:
		XBeeByteStream bstream;
		bool sop_seen;
		uint16_t length;
		uint8_t length_seen;
		std::vector<uint8_t> buffer;
		void on_sop();
		void on_bytes(const void *, std::size_t);
};

#endif

