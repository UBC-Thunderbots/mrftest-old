#include "util/config.h"
#include "util/dprint.h"
#include "util/time.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <glibmm.h>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	const struct BAUD_INFO {
		unsigned int display;
		speed_t tc;
	} BAUDS[] = {
		{ 9600, B9600 },
		{ 250000, B38400 },
		{ 115200, B115200 },
		{ 57600, B57600 },
		{ 1200, B1200 },
		{ 2400, B2400 },
		{ 19200, B19200 },
		{ 38400, B38400 },
	};

	void usage(const char *app) {
		std::cerr << "Usage:\n\n" << app << " [port]\n\n";
	}

	void send_fully(int fd, const void *data, std::size_t length) {
		const char *ptr = static_cast<const char *>(data);
		while (length) {
			ssize_t rc = write(fd, ptr, length);
			if (rc > 0) {
				ptr += rc;
				length -= rc;
			} else {
				std::perror("write");
				std::exit(1);
			}
		}
	}

	void send_string(int fd, const std::string &str) {
		send_fully(fd, str.data(), str.size());
	}

	bool read_messy_ok(int fd) {
		timespec end_time, cur_time;
		timespec_now(end_time);
		end_time.tv_sec++;

		bool seen_O = false;
		bool seen_K = false;
		for (;; ) {
			char ch;
			ssize_t rc;

			timespec_now(cur_time);
			if (cur_time.tv_sec > end_time.tv_sec || (cur_time.tv_sec == end_time.tv_sec && cur_time.tv_nsec > end_time.tv_nsec)) {
				return false;
			}
			rc = read(fd, &ch, 1);
			if (rc < 0) {
				std::perror("read");
				std::exit(1);
			} else if (rc == 1) {
				if (seen_O && seen_K && ch == '\r') {
					return true;
				} else if (seen_O && ch == 'K') {
					seen_O = true;
					seen_K = true;
				} else if (ch == 'O') {
					seen_O = true;
					seen_K = false;
				} else {
					seen_O = false;
					seen_K = false;
				}
			}
		}
	}

	std::string read_line(int fd) {
		timespec end_time, cur_time;
		timespec_now(end_time);
		end_time.tv_sec++;

		std::string line;
		for (;; ) {
			char ch;
			ssize_t rc;

			timespec_now(cur_time);
			if (cur_time.tv_sec > end_time.tv_sec || (cur_time.tv_sec == end_time.tv_sec && cur_time.tv_nsec > end_time.tv_nsec)) {
				return "";
			}
			rc = read(fd, &ch, 1);
			if (rc < 0) {
				std::perror("read");
				std::exit(1);
			} else if (rc == 1) {
				if (ch == '\r') {
					return line;
				} else {
					line += ch;
				}
			}
		}
	}

	bool read_clean_ok(int fd) {
		return read_line(fd) == "OK";
	}
}

int main(int argc, char **argv) {
	std::locale::global(std::locale(""));

	Config conf;

	const char *path;
	if (argc == 1) {
		path = "/dev/xbee";
	} else if (argc == 2) {
		path = argv[1];
	} else {
		usage(argv[0]);
		return 1;
	}

	int fd = open(path, O_RDWR);
	if (fd < 0) {
		std::perror("open");
		return 1;
	}

	std::cout << "Getting old extended serial parameters... " << std::flush;
	serial_struct old_serinfo;
	if (ioctl(fd, TIOCGSERIAL, &old_serinfo) < 0) {
		std::perror("ioctl");
		return 1;
	} else {
		std::cout << "OK\n";
	}

start_work:
	for (unsigned int baudidx = 0; baudidx < G_N_ELEMENTS(BAUDS); baudidx++) {
		for (unsigned int retries = 0; retries < 3; retries++) {
			std::cout << "Entering command mode at " << BAUDS[baudidx].display << " baud... " << std::flush;

			if (BAUDS[baudidx].display == 250000) {
				serial_struct new_serinfo = old_serinfo;
				new_serinfo.flags &= ~ASYNC_SPD_MASK;
				new_serinfo.flags |= ASYNC_SPD_CUST;
				new_serinfo.custom_divisor = new_serinfo.baud_base / 250000;
				if (ioctl(fd, TIOCSSERIAL, &new_serinfo) < 0) {
					std::perror("ioctl");
					break;
				}
			}

			termios tios;
			if (tcgetattr(fd, &tios) < 0) {
				std::perror("tcgetattr");
				return 1;
			}
			cfmakeraw(&tios);
			tios.c_cflag &= ~CRTSCTS;
			tios.c_cflag |= CSTOPB;
			tios.c_cc[VMIN] = 0;
			tios.c_cc[VTIME] = 5;
			cfsetispeed(&tios, BAUDS[baudidx].tc);
			cfsetospeed(&tios, BAUDS[baudidx].tc);
			if (tcsetattr(fd, TCSAFLUSH, &tios) < 0) {
				std::perror("tcsetattr");
				break;
			}

			timespec ts;
			ts.tv_sec = 1;
			ts.tv_nsec = 200000000;
			nanosleep(&ts, 0);

			send_string(fd, "+++");

			ts.tv_sec = 0;
			ts.tv_nsec = 850000000;
			nanosleep(&ts, 0);

			if (read_messy_ok(fd)) {
				goto connected;
			}
			std::cout << "FAIL\n";
		}
	}

	std::cout << "Giving up.\n";
	return 1;

connected:
	std::cout << "OK\n";

	std::cout << "Getting firmware version... " << std::flush;
	send_string(fd, "ATVR\r");
	const std::string &version_string = read_line(fd);
	if (version_string.empty()) {
		std::cout << "FAIL\n";
		goto start_work;
	}
	std::cout << "OK\n";
	std::istringstream iss_version(version_string);
	iss_version.setf(std::ios_base::hex, std::ios_base::basefield);
	unsigned int version;
	iss_version >> version;
	if (version < 0x10E8) {
		std::cout << "WARNING --- WARNING --- WARNING\nFirmware version is less than 10E8. Consider upgrading!\nWARNING --- WARNING --- WARNING\n";
	}

	std::cout << "Loading factory default settings... " << std::flush;
	send_string(fd, "ATRE\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting API mode with escapes... " << std::flush;
	send_string(fd, "ATAP2\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting serial baud rate to 250,000... " << std::flush;
	send_string(fd, "ATBD3D090\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Enabling RTS flow control..." << std::flush;
	send_string(fd, "ATD61\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting DIO0 to default as digital output low... " << std::flush;
	send_string(fd, "ATD04\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting radio channel to " << tohex(conf.channel(), 2) << "... " << std::flush;
	{
		send_string(fd, Glib::ustring::compose("ATCH%1\r", tohex(conf.channel(), 1)));
	}
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting PAN ID to 496C... " << std::flush;
	send_string(fd, "ATID496C\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting 16-bit address to none... " << std::flush;
	send_string(fd, "ATMYFFFF\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting clear channel threshold to -44dBm... " << std::flush;
	send_string(fd, "ATCA2C\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Saving configuration... " << std::flush;
	send_string(fd, "ATWR\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Rebooting modem... " << std::flush;
	send_string(fd, "ATFR\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\nThe modem did not reboot. Please power cycle it.\n";
	}

	std::cout << "Restoring original serial configuration... " << std::flush;
	if (ioctl(fd, TIOCSSERIAL, &old_serinfo) < 0) {
		std::perror("ioctl");
	} else {
		std::cout << "OK\n";
	}

	std::cout << "===================\nOPERATION SUCCEEDED\n===================\n";

	return 0;
}

