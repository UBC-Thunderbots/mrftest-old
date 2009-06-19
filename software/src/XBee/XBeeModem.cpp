#include "datapool/Config.h"
#include "Log/Log.h"
#include "XBee/XBeeModem.h"

#include <vector>
#include <string>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <stdint.h>

#include <sigc++/sigc++.h>
#include <glibmm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

namespace {
	bool readEnterCommandModeOK(int fd) {
		// Set timeout: the OK should arrive right away, so give it around a third of a second to arrive.
		// We do three read attempts at 1/10 of a second each.
		termios tios;
		if (tcgetattr(fd, &tios) < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot get serial parameters: " << std::strerror(err) << '\n';
			std::exit(1);
		}
		tios.c_cc[VMIN] = 0;
		tios.c_cc[VTIME] = 1;
		if (tcsetattr(fd, TCSANOW, &tios) < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot set serial timeout: " << std::strerror(err) << '\n';
			std::exit(1);
		}

		// This might have other data overlaid, so we need to flush through the data and try to find an OK at the end.
		char buffer[1024];
		unsigned int wptr = 0;

		for (unsigned int i = 0; i < 3 && wptr < sizeof(buffer); i++) {
			ssize_t ret = read(fd, &buffer[wptr], sizeof(buffer) - wptr);
			if (ret < 0) {
				int err = errno;
				Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot read from serial port: " << std::strerror(err) << '\n';
				std::exit(1);
			} else if (ret > 0) {
				wptr += ret;
				if (wptr >= 3 && memcmp("OK\r", &buffer[wptr - 3], 3) == 0) {
					return true;
				}
			}
		}

		return false;
	}

	bool readOK(int fd) {
		// This is easy: just set a timeout of a second and try to read the OK.
		termios tios;
		if (tcgetattr(fd, &tios) < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot get serial parameters: " << std::strerror(err) << '\n';
			std::exit(1);
		}
		tios.c_cc[VMIN] = 0;
		tios.c_cc[VTIME] = 10;
		if (tcsetattr(fd, TCSANOW, &tios) < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot set serial timeout: " << std::strerror(err) << '\n';
			std::exit(1);
		}

		// Do the read.
		char buffer[32];
		ssize_t ret = read(fd, buffer, sizeof(buffer));
		if (ret < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot read from serial port: " << std::strerror(err) << '\n';
			std::exit(1);
		} else if (ret >= 3 && memcmp("OK\r", &buffer[ret - 3], 3) == 0) {
			return true;
		}

		return false;
	}

	void writeFully(int fd, const void *data, std::size_t len) {
		const char *dptr = reinterpret_cast<const char *>(data);
		while (len) {
			ssize_t ret = write(fd, dptr, len);
			if (ret < 0) {
				int err = errno;
				Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot write to serial port: " << std::strerror(err) << '\n';
				std::exit(1);
			}
			dptr += ret;
			len -= ret;
		}
	}

	void writeString(int fd, const std::string &s) {
		writeFully(fd, s.data(), s.size());
	}
}

XBeeModem::XBeeModem() : rxBufPtr(0) {
	// Get the device path from the config file.
	const std::string &device = Config::instance().getString("XBee", "Device");

	// Open serial port.
	fd = open(device.c_str(), O_RDWR | O_NOCTTY);
	if (fd < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot open " << device << ": " << std::strerror(err) << '\n';
		std::exit(1);
	}

	// Move into command mode by sending +++. Try different baud rates.
	bool inCommandMode = false;
	static const speed_t BAUDS[] = {B115200, B9600, B1200, B2400, B4800, B19200, B38400, B57600};
	static const unsigned int BAUDNAMES[] = {115200, 9600, 1200, 2400, 4800, 19200, 38400, 57600};
	for (unsigned int i = 0; i < sizeof(BAUDS) / sizeof(*BAUDS) && !inCommandMode; i++) {
		// Print message.
		Log::log(Log::LEVEL_INFO, "XBee") << "Connecting at " << BAUDNAMES[i] << " baud...\n";

		// Initialize TTY settings.
		termios tios;
		if (tcgetattr(fd, &tios) < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot get serial parameters: " << std::strerror(err) << '\n';
			std::exit(1);
		}
		tios.c_iflag = 0;
		tios.c_oflag = NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
		tios.c_cflag = CS8 | CSTOPB | CREAD | HUPCL | CRTSCTS;
		tios.c_lflag = 0;
		cfsetispeed(&tios, BAUDS[i]);
		cfsetospeed(&tios, BAUDS[i]);
		if (tcsetattr(fd, TCSAFLUSH, &tios) < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot set serial parameters: " << std::strerror(err) << '\n';
			std::exit(1);
		}

		// Try twice to connect.
		for (unsigned int j = 0; j < 2 && !inCommandMode; j++) {
			// Wait a bit.
			struct timespec ts;
			ts.tv_sec = 1;
			ts.tv_nsec = 200000000;
			nanosleep(&ts, 0);
			// Send +++.
			writeString(fd, "+++");
			// Wait a bit.
			ts.tv_sec = 1;
			ts.tv_nsec = 200000000;
			nanosleep(&ts, 0);
			// Try to read a line.
			inCommandMode = readEnterCommandModeOK(fd);
		}
	}

	// Reset to factory configuration.
	do {
		writeString(fd, "ATRE\r");
	} while (!readOK(fd));

	// Switch into non-escaped API mode by sending ATAP1\r
	do {
		writeString(fd, "ATAP1\r");
	} while (!readOK(fd));

	// Set baud rate.
	do {
		writeString(fd, "ATBD7\r");
	} while (!readOK(fd));

	// Switch out of command mode by sending ATCN\r
	do {
		writeString(fd, "ATCN\r");
	} while (!readOK(fd));

	// Raise serial port baud rate and switch to nonblocking IO.
	{
		termios tios;
		if (tcgetattr(fd, &tios) < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot get serial parameters: " << std::strerror(err) << '\n';
			std::exit(1);
		}
		tios.c_iflag = 0;
		tios.c_oflag = NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
		tios.c_cflag = CS8 | CSTOPB | CREAD | HUPCL | CRTSCTS;
		tios.c_lflag = 0;
		tios.c_cc[VMIN] = 1;
		tios.c_cc[VTIME] = 0;
		cfsetispeed(&tios, B115200);
		cfsetospeed(&tios, B115200);
		if (tcsetattr(fd, TCSAFLUSH, &tios) < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot set serial parameters: " << std::strerror(err) << '\n';
			std::exit(1);
		}
		long flags = fcntl(fd, F_GETFL);
		if (flags < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot get serial port flags: " << std::strerror(err) << '\n';
			std::exit(1);
		}
		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Cannot set serial port to nonblocking: " << std::strerror(err) << '\n';
			std::exit(1);
		}
	}

	Log::log(Log::LEVEL_INFO, "XBee") << "Initialized OK.\n";
	Glib::signal_io().connect(sigc::mem_fun(*this, &XBeeModem::onIO), fd, Glib::IO_IN);
}

XBeeModem::~XBeeModem() {
	close(fd);
}

void XBeeModem::send(uint8_t type, const void *payload, std::size_t payloadSize) {
	if (payloadSize > 65535) {
		Log::log(Log::LEVEL_ERROR, "XBee") << "Payload size " << payloadSize << " too big.\n";
		std::exit(1);
	}

	uint8_t buffer[payloadSize + 5];
	buffer[0] = 0x7E;
	buffer[1] = (payloadSize + 1) / 256;
	buffer[2] = (payloadSize + 1) % 256;
	buffer[3] = type;
	uint8_t checksum = 0xFFU - type;
	const unsigned char *payloadPtr = reinterpret_cast<const unsigned char *>(payload);
	for (unsigned int i = 0; i < payloadSize; i++) {
		buffer[i + 4] = payloadPtr[i];
		checksum -= payloadPtr[i];
	}
	buffer[sizeof(buffer) - 1] = checksum;
	writeFully(fd, buffer, sizeof(buffer));
}

sigc::connection XBeeModem::connect_packet_received(uint8_t type, sigc::slot<void, const void *, std::size_t> slot) {
	return signal_packet_received[type].connect(slot);
}

bool XBeeModem::onIO(Glib::IOCondition events) {
	if ((events & Glib::IO_ERR) || (events & Glib::IO_NVAL) || (events & Glib::IO_HUP)) {
		Log::log(Log::LEVEL_ERROR, "XBee") << "Error on serial port.\n";
		std::exit(1);
	}

	if (events & Glib::IO_IN) {
		ssize_t ret = read(fd, &rxBuf[rxBufPtr], sizeof(rxBuf) - rxBufPtr);
		if (ret < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
			return true;
		} else if (ret < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "Error reading serial port: " << std::strerror(err) << '\n';
			std::exit(1);
		}
		rxBufPtr += ret;

		for (;;) {
			if (rxBufPtr > 0 && rxBuf[0] != 0x7E) {
				// Mis-synchronized. Is there another 7E in the stream?
				Log::log(Log::LEVEL_WARNING, "XBee") << "Serial stream synchronization lost.\n";
				const uint8_t *p = reinterpret_cast<const uint8_t *>(std::memchr(rxBuf, 0x7E, rxBufPtr));
				if (p) {
					std::memmove(rxBuf, p, rxBufPtr - (p - rxBuf));
					rxBufPtr -= p - rxBuf;
				} else {
					rxBufPtr = 0;
				}
			}

			if (!rxBufPtr) {
				// Buffer is empty.
				return true;
			}

			// We have the start of a packet. We can find the length only if we have at least 3 bytes.
			if (rxBufPtr < 3) {
				return true;
			}

			// Extract length.
			unsigned int payloadLength = rxBuf[1] * 256 + rxBuf[2];

			// Check if we have the full packet.
			if (rxBufPtr < payloadLength + 4) {
				return true;
			}

			// Extract payload.
			const uint8_t *payload = &rxBuf[3];

			// Checksum the payload.
			uint8_t checksum = 0xFF;
			for (unsigned int i = 0; i < payloadLength; i++)
				checksum -= payload[i];
			if (checksum == payload[payloadLength]) {
				uint8_t type = payload[0];
				signal_packet_received[type](payload + 1, payloadLength - 1);
			} else {
				Log::log(Log::LEVEL_WARNING, "XBee") << "Received packet with bad checksum.\n";
			}
			std::memmove(&rxBuf, &rxBuf[payloadLength + 4], rxBufPtr - (payloadLength + 4));
			rxBufPtr -= payloadLength + 4;
		}
	}

	return true;
}

