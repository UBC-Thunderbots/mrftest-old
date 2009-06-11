#include "XBee/XBee.h"
#include "Log/Log.h"
#include "datapool/Team.h"

#include <fstream>
#include <string>
#include <exception>
#include <algorithm>
#include <vector>
#include <list>
#include <sstream>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <climits>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#define POWER_LEVEL "4"
#define TIMEOUT     1000 // milliseconds (for serial reads)
#define ACK_TIMEOUT 100  // milliseconds (for packet ACKs during normal run)
#define RCV_TIMEOUT 5000 // milliseconds (for feedback data from the bot)
#define TX_MAXERR   4    // maximum # errors on transmit before reporting error
#define KICK_TIME   150  // length of time to send kicker-active packets before clearing
#define REBOOT_TIME 1000 // length of time to send reboot packets before clearing

namespace {
	/*
	 * Gets the current timestamp, in milliseconds.
	 */
	unsigned long millis() {
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		unsigned long ans = ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL;
		return ans ? ans : 1;
	}

	/*
	 * Calculates how many milliseconds to wait until the specified length of
	 * time is reached from the specified start time.
	 */
	unsigned long timeUntil(unsigned long length, unsigned long start) {
		unsigned long now = millis();
		if (now > start + length)
			return 0UL;
		else
			return start + length - now;
	}

	/*
	 * A packet used with the API-mode interface.
	 */
	class packet {
		public:
			packet() {
			}

			packet(std::size_t payloadLength) : buffer(payloadLength + 4) {
			}

			packet(const std::vector<unsigned char> &data) {
				assert(data.size() > 3);
				std::size_t payloadLength = data[1] * 256 + data[2];
				assert(data.size() >= payloadLength + 4);
				buffer.resize(payloadLength + 4);
				std::copy(&data[0], &data[payloadLength + 4], &buffer[0]);
			}

			bool verify() {
				if (buffer[0] != 0x7E)
					return false;
				if (buffer.size() - 4 != buffer[1] * 256 + buffer[2])
					return false;
				unsigned char checksum = 0;
				for (unsigned int i = 3; i < buffer.size(); i++)
					checksum += buffer[i];
				if (checksum != 0xFF)
					return false;
				return true;
			}

			unsigned char &operator[](unsigned int index) {
				return buffer[index + 3];
			}

			unsigned char operator[](unsigned int index) const {
				return buffer[index + 3];
			}

			std::size_t payloadSize() const {
				return buffer.size() - 4;
			}

			void prewrite() {
				buffer[0] = 0x7E;
				std::size_t payloadLength = buffer.size() - 4;
				assert(payloadLength <= 0xFFFFU);
				buffer[1] = payloadLength / 256;
				buffer[2] = payloadLength % 256;
				unsigned char checksum = 0;
				for (unsigned int i = 3; i < buffer.size() - 1; i++)
					checksum += buffer[i];
				buffer[buffer.size() - 1] = 0xFFU - checksum;
			}

			const unsigned char *data() const {
				return &buffer[0];
			}

			std::size_t dataSize() const {
				return buffer.size();
			}

			void dataSize(std::size_t len) {
				buffer.resize(len);
			}

		private:
			std::vector<unsigned char> buffer;
	};

	// The file descriptor.
	int serialPort = -1;

	// The channel number.
	unsigned short channel;

	// The PAN number.
	unsigned short pan;

	// The addresses of the robots.
	uint64_t botAddresses[Team::SIZE];

	// Bytes received from the serial port that are not yet part of a full packet.
	std::vector<unsigned char> receivedBytes;

	// Full packets received that have not yet been accepted by an upper layer, e.g. due to type filtering.
	std::list<packet> receivedPackets;

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
				Log::log(Log::LEVEL_ERROR, "XBee") << "poll: " << std::strerror(err) << '\n';
				std::exit(1);
			}

			ssize_t ret = ::write(serialPort, dptr, length);
			if (ret < 0) {
				if (errno == EINTR || errno == EAGAIN) {
					ret = 0;
				} else {
					int err = errno;
					Log::log(Log::LEVEL_ERROR, "XBee") << "write: " << std::strerror(err) << '\n';
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
	 * Writes a packet to the serial port.
	 */
	void writeFully(const packet &pkt) {
		writeFully(pkt.data(), pkt.dataSize());
	}

	/*
	 * Reads from the serial port, expecting to see "OK\r".
	 * On success, returns true.
	 * On failure (timeout without receiving "OK\r"), returns false.
	 */
	bool readOK() {
		unsigned long startTime = millis();
		unsigned char buffer[3] = {0, 0, 0};
		pollfd pfd;
		pfd.fd = serialPort;
		pfd.events = POLLIN;
		pfd.revents = 0;

		while (buffer[0] != 'O' || buffer[1] != 'K' || buffer[2] != '\r') {
			{
				int ret = ::poll(&pfd, 1, timeUntil(TIMEOUT, startTime));
				if (ret < 0) {
					int err = errno;
					Log::log(Log::LEVEL_ERROR, "XBee") << "poll: " << std::strerror(err) << '\n';
					std::exit(1);
				} else if (ret == 0) {
					Log::log(Log::LEVEL_WARNING, "XBee") << "read: timed out\n";
					return false;
				}
			}
			{
				char ch;
				ssize_t ret = ::read(serialPort, &ch, 1);
				if (ret < 0) {
					if (errno != EINTR) {
						int err = errno;
						Log::log(Log::LEVEL_ERROR, "XBee") << "read: " << std::strerror(err) << '\n';
						std::exit(1);
					}
				} else if (ret == 1) {
					buffer[0] = buffer[1];
					buffer[1] = buffer[2];
					buffer[2] = ch;
				}
			}
		}

		return true;
	}

	/*
	 * Tries to receive some data from the serial port.
	 * Does not wait at all: only already-received data is read.
	 * On success, returns true.
	 * On timeout, returns false.
	 */
	bool iterateReceive() {
		unsigned char tmp[32];
		ssize_t ret;

		do {
			ret = ::read(serialPort, tmp, sizeof(tmp));
		} while (ret < 0 && errno == EINTR);

		if (ret < 0 && errno == EAGAIN) {
			return false;
		} else if (ret < 0) {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "XBee") << "read: " << strerror(err) << '\n';
			std::exit(1);
		} else if (ret == 0) {
			return false;
		} else {
			std::vector<unsigned char>::size_type idx = receivedBytes.size();
			receivedBytes.resize(idx + ret);
			std::copy(tmp, tmp + ret, &receivedBytes[idx]);
			return true;
		}
	}

	/*
	 * Tries to extract a complete packet from the received bytes buffer.
	 * On success, returns true.
	 * If no full packet available yet, returns false.
	 */
	bool iteratePacketize() {
		if (receivedBytes.size() < 3)
			return false;

		std::size_t payloadLength = receivedBytes[1] * 256 + receivedBytes[2];
		if (receivedBytes.size() < payloadLength + 4)
			return false;

		packet pkt(receivedBytes);
		receivedBytes.erase(receivedBytes.begin(), receivedBytes.begin() + pkt.dataSize());
		receivedPackets.push_back(pkt);
		return true;
	}

	/*
	 * Removes a packet from the queue.
	 * If type == 0xFF, returns any packet.
	 * If type != 0xFF, returns only a packet with that type.
	 * On success, returns true.
	 * On no such packet, returns false.
	 */
	bool findPacket(packet &pkt, unsigned char type) {
		for (std::list<packet>::iterator i = receivedPackets.begin(), iend = receivedPackets.end(); i != iend; ++i) {
			const packet &cur = *i;
			if (type == 0xFF || cur[0] == type) {
				pkt = cur;
				receivedPackets.erase(i);
				return true;
			}
		}

		return false;
	}

	/*
	 * Reads a packet from the serial port.
	 * If type == 0xFF, returns any packet.
	 * If type != 0xFF, returns only a packet with that type.
	 * If wait == true, wait up to the timeout before returning.
	 * If wait == false, only read already-received data.
	 * On success, returns true.
	 * On timeout, returns false.
	 */
	bool readPacket(packet &pkt, unsigned char type, bool wait) {
		unsigned long startTime = millis();
		pollfd pfd;
		pfd.fd = serialPort;
		pfd.events = POLLIN;
		pfd.revents = 0;

		for (;;) {
			// First try pulling some data from the kernel's buffer and packetizing it.
			while (iterateReceive())
				iteratePacketize();

			// If there's already a packet of the needed type, just return it.
			if (findPacket(pkt, type))
				return true;

			// If we're in no-wait mode, return now.
			if (!wait)
				return false;

			// Poll for more data.
			int ret = ::poll(&pfd, 1, timeUntil(TIMEOUT, startTime));
			if (ret < 0) {
				int err = errno;
				Log::log(Log::LEVEL_ERROR, "XBee") << "poll: " << strerror(err) << '\n';
				std::exit(1);
			} else if (ret == 0) {
				// Timeout.
				return false;
			}
		}
	}

	// The timestamp when a frame was sent to the XBee that has not yet seen a response (0 if none outstanding).
	unsigned long txTimestamp[Team::SIZE];

	// The number of packets that have timed out or errored to this particular bot.
	unsigned int txErrors[Team::SIZE];

	// The timestamp when a frame was last received from a particular bot (0 if none received yet).
	unsigned long rxTimestamp[Team::SIZE];

	// The timestamp when a robot's kicker was first told to be activated (0 if not activated yet).
	unsigned long kickTimestamp[Team::SIZE];

	// The timestamp when a robot was told to reboot.
	unsigned long rebootTimestamp[Team::SIZE];
}

XBee::CommStatus XBee::commStatus[Team::SIZE];
XBee::TXData XBee::out[Team::SIZE];
XBee::RXData XBee::in[Team::SIZE];

void XBee::init() {
	// Close port if already open.
	if (serialPort >= 0) {
		::close(serialPort);
		serialPort = -1;
	}

	// Clear buffers.
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		txTimestamp[i]     = 0;
		txErrors[i]        = 0;
		rxTimestamp[i]     = 0;
		kickTimestamp[i]   = 0;
		rebootTimestamp[i] = 0;
		commStatus[i]      = STATUS_FAILED;
		out[i].vx          = 0;
		out[i].vy          = 0;
		out[i].vt          = 0;
		out[i].dribble     = 0;
		out[i].kick        = 0;
		out[i].emergency   = 0xFF;
		out[i].vxMeasured  = -128;
		out[i].vyMeasured  = -128;
		out[i].reboot      = 0;
		out[i].extra       = -128;
		in[i].vGreen[0]    = 0xFF;
		in[i].vGreen[1]    = 0xFF;
		in[i].vMotor[0]    = 0xFF;
		in[i].vMotor[1]    = 0xFF;
	}

	// Load configuration file.
	std::string device;
	{
		const char *homedir = std::getenv("HOME");
		if (!homedir) {
			Log::log(Log::LEVEL_ERROR, "XBee") << "Environment variable $HOME is not set!\n";
			std::exit(1);
		}
		std::string configfile(homedir);
		configfile += "/.thunderbots/xbee.conf";
		std::ifstream ifs(configfile.c_str());
		ifs.setf(std::ios_base::hex, std::ios_base::basefield);

		ifs >> device;
		ifs >> channel;
		ifs >> pan;

		for (unsigned int i = 0; i < Team::SIZE; i++) {
			ifs >> botAddresses[i];
		}

		if (!ifs.good()) {
			std::ostream &os = Log::log(Log::LEVEL_ERROR, "XBee");
			os << "Error reading file " << configfile << "; does it exist and is it properly formatted?\n";
			os << "It should contain the path to the TTY followed by seven hex numbers separated by whitespace:\n";
			os << "- Channel number\n";
			os << "- PAN number\n";
			os << "- (5) 64-bit factory-assigned addresses of robot XBees\n";
			std::exit(1);
		}
	}

	// Print message.
	Log::log(Log::LEVEL_INFO, "XBee") << "Initializing...\n";

	// Open serial port.
	serialPort = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (serialPort < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "XBee") << "open(" << device << "): " << strerror(err) << '\n';
		std::exit(1);
	}

	bool commandModeOK = false;
	while (!commandModeOK) {
		static const speed_t BAUDS[] = {B115200, B9600, B1200, B2400, B4800, B19200, B38400, B57600};
		static const unsigned int BAUDNAMES[] = {115200, 9600, 1200, 2400, 4800, 19200, 38400, 57600};
		for (unsigned int baudidx = 0; baudidx < sizeof(BAUDS)/sizeof(*BAUDS) && !commandModeOK; baudidx++) {
			Log::log(Log::LEVEL_DEBUG, "XBee") << "Connecting at " << BAUDNAMES[baudidx] << " baud...\n";

			// Initialize TTY settings.
			termios tios;
			if (tcgetattr(serialPort, &tios) < 0) {
				Log::log(Log::LEVEL_ERROR, "XBee") << "tcgetattr() failed\n";
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
				Log::log(Log::LEVEL_ERROR, "XBee") << "tcsetattr() failed\n";
				std::exit(1);
			}

			// Switch into command mode by sending +++
			sleep(1);
			writeFully("+++");
			sleep(1);
			commandModeOK = readOK();
		}
	}

	// Reset to factory configuration.
	do {
		writeFully("ATRE\r");
	} while (!readOK());

	// Switch into non-escaped API mode by sending ATAP1\r
	do {
		writeFully("ATAP1\r");
	} while (!readOK());

	// Set power level.
	do {
		writeFully("ATPL" POWER_LEVEL "\r");
	} while (!readOK());

	// Set channel.
	do {
		std::ostringstream oss;
		oss.setf(std::ios_base::hex, std::ios_base::basefield);
		oss << "ATCH" << channel << '\r';
		writeFully(oss.str());
	} while (!readOK());

	// Set PAN.
	do {
		std::ostringstream oss;
		oss.setf(std::ios_base::hex, std::ios_base::basefield);
		oss << "ATID" << pan << '\r';
		writeFully(oss.str());
	} while (!readOK());

	// Set local address.
	do {
		writeFully("ATMYFFFF\r");
	} while (!readOK());

	// Set baud rate.
	do {
		writeFully("ATBD7\r");
	} while (!readOK());

	// Switch out of command mode by sending ATCN\r
	do {
		writeFully("ATCN\r");
	} while (!readOK());

	// Raise serial port baud rate.
	{
		termios tios;
		if (tcgetattr(serialPort, &tios) < 0) {
			Log::log(Log::LEVEL_ERROR, "XBee") << "tcgetattr() failed\n";
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
		if (tcsetattr(serialPort, TCSAFLUSH, &tios) < 0) {
			Log::log(Log::LEVEL_ERROR, "XBee") << "tcsetattr() failed\n";
			std::exit(1);
		}
	}

	Log::log(Log::LEVEL_INFO, "XBee") << "Initialized OK.\n";
}

void XBee::update() {
	{
		// Receive and handle any inbound data.
		packet pkt;
		while (readPacket(pkt, 0xFF, false)) {
			if (pkt.verify()) {
				if (pkt[0] == 0x80) {
					// Received data.
					uint64_t sender = 0;
					for (unsigned int i = 0; i < 8; i++)
						sender = (sender << 8) | pkt[i + 1];
					unsigned int bot = UINT_MAX;
					for (unsigned int i = 0; i < Team::SIZE; i++)
						if (sender && sender == botAddresses[i])
							bot = i;
					if (bot < Team::SIZE) {
						if (pkt.payloadSize() == 11 + sizeof(RXData)) {
							unsigned char *ptr = reinterpret_cast<unsigned char *>(&in[bot]);
							for (unsigned int i = 0; i < sizeof(RXData); i++)
								ptr[i] = pkt[11 + i];
							rxTimestamp[bot] = millis();
							if (commStatus[bot] == STATUS_NO_RECV)
								commStatus[bot] = STATUS_OK;
						} else {
							Log::log(Log::LEVEL_WARNING, "XBee") << "Received malformed packet from bot\n";
						}
					} else {
						Log::log(Log::LEVEL_WARNING, "XBee") << "Received packet from unknown sender\n";
					}
				} else if (pkt[0] == 0x8A) {
					// Modem status.
					switch (pkt[1]) {
						case 0:  Log::log(Log::LEVEL_WARNING, "XBee") << "Hardware reset\n"; break;
						case 1:  Log::log(Log::LEVEL_WARNING, "XBee") << "WDT reset\n"; break;
						case 4:  Log::log(Log::LEVEL_WARNING, "XBee") << "Synchronization lost\n"; break;
						default: Log::log(Log::LEVEL_WARNING, "XBee") << "Unknown modem status\n"; break;
					}
				} else if (pkt[0] == 0x89) {
					// Transmit status.
					if (pkt[1] >= 1 && pkt[1] <= Team::SIZE && txTimestamp[pkt[1] - 1]) {
						unsigned int bot = pkt[1] - 1;
						switch (pkt[2]) {
							case 0: // Reception OK.
								if (commStatus[bot] == STATUS_FAILED)
									commStatus[bot] = STATUS_NO_RECV;
								txErrors[bot] = 0;
								break;

							case 1: // No acknowledgement.
							case 2: // Clear channel availability failed.
								txErrors[bot]++;
								if (txErrors[bot] >= TX_MAXERR) {
									txErrors[bot] = TX_MAXERR;
									commStatus[bot] = STATUS_FAILED;
								}
								break;

							default:
								Log::log(Log::LEVEL_WARNING, "XBee") << "Unknown transmit packet status\n"; break;
						}
						txTimestamp[bot] = 0;
					} else {
						Log::log(Log::LEVEL_WARNING, "XBee") << "Gratuitious ACK\n";
					}
				} else {
					Log::log(Log::LEVEL_WARNING, "XBee") << "Unknown packet type\n";
				}
			} else {
				Log::log(Log::LEVEL_WARNING, "XBee") << "Malformed packet received\n";
			}
		}
	}

	{
		// Check for timeouts.
		for (unsigned int i = 0; i < Team::SIZE; i++) {
			if (txTimestamp[i] && millis() - txTimestamp[i] > ACK_TIMEOUT) {
				txTimestamp[i] = 0;
				txErrors[i]++;
				if (txErrors[i] >= TX_MAXERR) {
					txErrors[i] = TX_MAXERR;
					commStatus[i] = STATUS_FAILED;
				}
			}

			if (rxTimestamp[i] && millis() - rxTimestamp[i] > RCV_TIMEOUT) {
				Log::log(Log::LEVEL_DEBUG, "XBee") << "Timeout receiving packet from " << i << '\n';
				rxTimestamp[i] = 0;
				if (commStatus[i] == STATUS_OK)
					commStatus[i] = STATUS_NO_RECV;
			}
		}
	}

	// Check if we need to start any one-shot operations.
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		if (!kickTimestamp[i] && out[i].kick)
			kickTimestamp[i] = millis();
		if (!rebootTimestamp[i] && out[i].reboot)
			rebootTimestamp[i] = millis();
	}

	// Check if we need to clear any one-shot operations.
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		if (kickTimestamp[i] && millis() - kickTimestamp[i] > KICK_TIME) {
			out[i].kick = 0;
			kickTimestamp[i] = 0;
		}
		if (rebootTimestamp[i] && millis() - rebootTimestamp[i] > REBOOT_TIME) {
			out[i].reboot = 0;
			rebootTimestamp[i] = 0;
		}
	}

	// Check for packets that want to be transmitted where there is nothing outstanding.
	for (unsigned int i = 0; i < Team::SIZE; i++) {
		if (!txTimestamp[i] && botAddresses[i]) {
			packet pkt(sizeof(out[i]) + 11);
			pkt[0] = 0x00;
			pkt[1] = i + 1;
			for (unsigned int j = 0; j < 8; j++)
				pkt[j + 2] = botAddresses[i] >> ((7 - j) * 8);
			pkt[10] = 0x00;
			const unsigned char *dptr = reinterpret_cast<const unsigned char *>(&out[i]);
			for (unsigned int j = 0; j < sizeof(out[i]); j++)
				pkt[j + 11] = dptr[j];
			pkt.prewrite();

			writeFully(pkt.data(), pkt.dataSize());
			txTimestamp[i] = millis();

			// Check if we need to update either of the one-shot timestamps.
			if (out[i].kick && !kickTimestamp[i])
				kickTimestamp[i] = millis();
			if (out[i].reboot && !rebootTimestamp[i])
				rebootTimestamp[i] = millis();
		}
	}
}

