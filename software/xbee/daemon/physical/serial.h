#ifndef XBEE_DAEMON_PHYSICAL_SERIAL_H
#define XBEE_DAEMON_PHYSICAL_SERIAL_H

#include "util/fd.h"
#include "util/noncopyable.h"
#include <cstddef>
#include <glibmm.h>
#include <stdint.h>
#include <sigc++/sigc++.h>
#include <sys/uio.h>

/**
 * A serial port running at 250,000 baud.
 */
class SerialPort : public NonCopyable, public sigc::trackable {
	public:
		/*
		 * Constructs a new SerialPort.
		 * Opens but does not configure the port.
		 */
		SerialPort();

		/**
		 * Configures the port.
		 */
		void configure_port();

		/**
		 * Returns the signal invoked when a byte arrives on the port.
		 *
		 * \return the signal.
		 */
		sigc::signal<void, const void *, std::size_t> &signal_received() {
			return sig_received;
		}

		/**
		 * Sends a string of bytes to the port.
		 *
		 * \param[in] iov a pointer to an array of iovecs to gather to find the bytes to send.
		 *
		 * \param[in] iovcnt the number of iovecs in the \p iov array.
		 */
		void send(iovec *iov, std::size_t iovcnt);

	private:
		const FileDescriptor::Ptr port;
		sigc::signal<void, const void *, std::size_t> sig_received;
		bool on_readable(Glib::IOCondition);
};

#endif

