#include <iostream>
#include <cstddef>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#define TIMEOUT     1000 // milliseconds (for serial reads)
#define ACK_TIMEOUT 100  // milliseconds (for packet ACKs during normal run)

namespace {
	// The file descriptor.
	int serialPort = -1;

	/*
	 * Writes all of a block of data to the serial port.
	 */
	void writeFully(const void *data, std::size_t length) {
		const char *dptr = static_cast<const char *>(data);
		pollfd pfd;
		pfd.fd = serialPort;
		pfd.events = POLLOUT;
		pfd.revents = 0;
		while (length) {
			if (::poll(&pfd, 1, -1) < 0) {
				int err = errno;
				std::cerr << "poll: " << std::strerror(err) << '\n';
				std::exit(1);
			}

			ssize_t ret = ::write(serialPort, dptr, length);
			if (ret < 0) {
				if (errno == EINTR || errno == EAGAIN) {
					ret = 0;
				} else {
					int err = errno;
					std::cerr << "write: " << std::strerror(err) << '\n';
					std::exit(1);
				}
			}
			dptr += ret;
			length -= ret;
		}
	}

	/*
	 * Writes all of a string to the serial port.
	 */
	void writeFully(const std::string &str) {
		writeFully(str.data(), str.size());
	}

	/*
	 * Reads a line from the serial port.
	 * On success, returns the line (minus the CR).
	 * On timeout, displays a message and returns the data so far.
	 */
	std::string readLine() {
		clock_t startTime = std::clock();
		std::string result;
		pollfd pfd;
		pfd.fd = serialPort;
		pfd.events = POLLIN;
		pfd.revents = 0;
		for (;;) {
			{
				int ret = ::poll(&pfd, 1, std::max(0L, TIMEOUT - (std::clock() - startTime) / (CLOCKS_PER_SEC / 1000)));
				if (ret < 0) {
					int err = errno;
					std::cerr << "poll: " << std::strerror(err) << '\n';
					std::exit(1);
				} else if (ret == 0) {
					std::cerr << "read: timed out\n";
					return result;
				}
			}
			{
				char ch;
				ssize_t ret = ::read(serialPort, &ch, 1);
				if (ret < 0) {
					if (errno != EINTR) {
						int err = errno;
						std::cerr << "read: " << std::strerror(err) << '\n';
						std::exit(1);
					}
				} else if (ret == 1) {
					if (ch == '\r')
						return result;
					else
						result += ch;
				}
			}
		}
	}
}

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cerr << "Usage:\n" << argv[0] << " ttydevice\n";
		return 1;
	}

	// Open serial port.
	serialPort = open(argv[1], O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (serialPort < 0) {
		int err = errno;
		std::cerr << "open(" << argv[1] << "): " << strerror(err) << '\n';
		std::exit(1);
	}

	std::string response;
	while (response != "OK") {
		static const speed_t BAUDS[] = {B9600, B57600, B1200, B2400, B4800, B19200, B38400, B115200};
		static const unsigned int BAUDNAMES[] = {9600, 57600, 1200, 2400, 4800, 19200, 38400, 115200};
		for (unsigned int baudidx = 0; baudidx < sizeof(BAUDS)/sizeof(*BAUDS) && response != "OK"; baudidx++) {
			std::cout << "Connecting at " << BAUDNAMES[baudidx] << "bps\n";

			// Initialize TTY settings.
			termios tios;
			if (tcgetattr(serialPort, &tios) < 0) {
				std::cerr << "tcgetattr() failed\n";
				std::exit(1);
			}

			tios.c_iflag = 0;
			tios.c_oflag = NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
			tios.c_cflag = CS8 | CSTOPB | CREAD | HUPCL | CRTSCTS;
			tios.c_lflag = 0;
			tios.c_cc[VMIN] = 1;
			tios.c_cc[VTIME] = 0;
			cfsetispeed(&tios, BAUDS[baudidx]);
			cfsetospeed(&tios, BAUDS[baudidx]);
			if (tcsetattr(serialPort, TCSAFLUSH, &tios) < 0) {
				std::cerr << "tcsetattr() failed\n";
				std::exit(1);
			}

			// Switch into command mode by sending +++
			unsigned int tries = 2;
			do {
				sleep(1);
				writeFully("+++");
				sleep(1);
				response = readLine();
			} while (response != "OK" && tries--);
		}
	}

	// Reset to factory configuration.
	do {
		writeFully("ATRE\r");
		response = readLine();
	} while (response != "OK");

	// Write to ROM.
	do {
		writeFully("ATWR\r");
		response = readLine();
	} while (response != "OK");

	std::cout << "Done!\n";
	return 0;
}

