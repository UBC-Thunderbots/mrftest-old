#ifndef XBEE_DAEMON_PHYSICAL_BYTEPROTO_H
#define XBEE_DAEMON_PHYSICAL_BYTEPROTO_H

#include "xbee/daemon/physical/serial.h"

/**
 * Performs the escaping necessary to provide a categorized byte stream to and from the XBee.
 */
class XBeeByteStream : public NonCopyable, public sigc::trackable {
	public:
		/**
		 * Constructs a new byte stream.
		 */
		XBeeByteStream();

		/**
		 * Configures the serial port.
		 */
		void configure_port() {
			port.configure_port();
		}

		/**
		 * Returns the signal invoked when the start-of-packet delimiter is received.
		 *
		 * \return the signal.
		 */
		sigc::signal<void> &signal_sop_received() {
			return sig_sop_received;
		}

		/**
		 * Returns the signal invoked when a byte is received.
		 *
		 * \return the signal.
		 */
		sigc::signal<void, const void *, std::size_t> &signal_bytes_received() {
			return sig_bytes_received;
		}

		/**
		 * Sends a start-of-packet delimiter to the port.
		 */
		void send_sop();

		/**
		 * Sends a string of bytes to the port.
		 *
		 * \param[in] iov a pointer to an array of iovecs to gather to find the bytes to send.
		 *
		 * \param[in] iovcnt the number of iovecs in the \p iov array.
		 */
		void send(const iovec *iov, std::size_t iovcnt);

	private:
		SerialPort port;
		sigc::signal<void> sig_sop_received;
		sigc::signal<void, const void *, std::size_t> sig_bytes_received;
		bool received_escape;
		void bytes_received(const void *, std::size_t);
};

#endif

