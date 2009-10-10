#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>

// Needed because Cygwin doesn't provide CLOCK_MONOTONIC.
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC CLOCK_REALTIME
#endif

namespace {
	const struct BAUD_INFO {
		unsigned int display;
		speed_t tc;
	} BAUDS[] = {
		{9600, B9600},
		{250000, B38400},
		{115200, B115200},
		{57600, B57600},
		{1200, B1200},
		{2400, B2400},
		{19200, B19200},
		{38400, B38400},
	};

	void usage(const char *app) {
		std::cerr << "Usage:\n\n" << app << " [port]\n\n";
	}

	void send_fully(int fd, const void *data, int length) {
		const char *ptr = reinterpret_cast<const char *>(data);
		while (length) {
			int rc = write(fd, ptr, length);
			if (rc > 0) {
				ptr += rc;
				length -= rc;
			} else {
				perror("write");
				std::exit(1);
			}
		}
	}

	void send_string(int fd, const std::string &str) {
		send_fully(fd, str.data(), str.size());
	}

	bool read_messy_ok(int fd) {
		timespec end_time, cur_time;
		if (clock_gettime(CLOCK_MONOTONIC, &end_time) < 0) {perror("clock_gettime"); std::exit(1);}
		end_time.tv_sec++;

		bool seen_O = false;
		bool seen_K = false;
		for (;;) {
			char ch;
			int rc;

			if (clock_gettime(CLOCK_MONOTONIC, &cur_time) < 0) {perror("clock_gettime"); std::exit(1);}
			if (cur_time.tv_sec > end_time.tv_sec || (cur_time.tv_sec == end_time.tv_sec && cur_time.tv_nsec > end_time.tv_nsec)) return false;
			rc = read(fd, &ch, 1);
			if (rc < 0) {perror("read"); std::exit(1);}
			else if (rc == 1) {
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
		if (clock_gettime(CLOCK_MONOTONIC, &end_time) < 0) {perror("clock_gettime"); std::exit(1);}
		end_time.tv_sec++;

		std::string line;
		for (;;) {
			char ch;
			int rc;

			if (clock_gettime(CLOCK_MONOTONIC, &cur_time) < 0) {perror("clock_gettime"); std::exit(1);}
			if (cur_time.tv_sec > end_time.tv_sec || (cur_time.tv_sec == end_time.tv_sec && cur_time.tv_nsec > end_time.tv_nsec)) return "";
			rc = read(fd, &ch, 1);
			if (rc < 0) {perror("read"); std::exit(1);}
			else if (rc == 1) {
				if (ch == '\r') return line;
				else line += ch;
			}
		}
	}

	bool read_clean_ok(int fd) {
		return read_line(fd) == "OK";
	}
}

int main(int argc, char **argv) {
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
	if (fd < 0) {perror("open"); return 1;}

	std::cout << "Getting old extended serial parameters... " << std::flush;
	serial_struct old_serinfo;
	if (ioctl(fd, TIOCGSERIAL, &old_serinfo) < 0) {
		perror("ioctl");
		return 1;
	} else {
		std::cout << "OK\n";
	}

start_work:
	for (unsigned int baudidx = 0; baudidx < sizeof(BAUDS) / sizeof(*BAUDS); baudidx++) {
		for (unsigned int retries = 0; retries < 3; retries++) {
			std::cout << "Entering command mode at " << BAUDS[baudidx].display << " baud... " << std::flush;

			if (BAUDS[baudidx].display == 250000) {
				serial_struct new_serinfo = old_serinfo;
				new_serinfo.flags &= ~ASYNC_SPD_MASK;
				new_serinfo.flags |= ASYNC_SPD_CUST;
				new_serinfo.custom_divisor = new_serinfo.baud_base / 250000;
				if (ioctl(fd, TIOCSSERIAL, &new_serinfo) < 0) {
					perror("ioctl");
					break;
				}
			}

			termios tios;
			if (tcgetattr(fd, &tios) < 0) {perror("tcgetattr"); return 1;}
			cfmakeraw(&tios);
			tios.c_cflag &= ~CRTSCTS;
			tios.c_cc[VMIN] = 0;
			tios.c_cc[VTIME] = 5;
			cfsetispeed(&tios, BAUDS[baudidx].tc);
			cfsetospeed(&tios, BAUDS[baudidx].tc);
			if (tcsetattr(fd, TCSAFLUSH, &tios) < 0) {perror("tcsetattr"); break;}

			timespec ts;
			ts.tv_sec = 1;
			ts.tv_nsec = 200000000;
			nanosleep(&ts, 0);

			send_string(fd, "+++");

			ts.tv_sec = 0;
			ts.tv_nsec = 850000000;
			nanosleep(&ts, 0);

			if (read_messy_ok(fd))
				goto connected;
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
	if (version < 0x10CD)
		std::cout << "WARNING: Firmware version is less than 10CD. Consider upgrading!\n";

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

	std::cout << "Setting radio channel... " << std::flush;
	send_string(fd, "ATCHE\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting DIO0 to default as digital output high... " << std::flush;
	send_string(fd, "ATD05\r");
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting PAN ID... " << std::flush;
	send_string(fd, "ATID6789\r");
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
		perror("ioctl");
	} else {
		std::cout << "OK\n";
	}

	return 0;
}

