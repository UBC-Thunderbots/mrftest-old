#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <getopt.h>
#include <time.h>

#ifndef CLOCK_MONOTONIC
	#define CLOCK_MONOTONIC CLOCK_REALTIME
#endif

namespace {
	const char SHORT_OPTIONS[] = "msh";

	const option LONG_OPTIONS[] = {
		{"master", no_argument, 0, 'm'},
		{"slave", no_argument, 0, 's'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	const struct BAUD_INFO {
		unsigned int display;
		speed_t tc;
	} BAUDS[] = {
		{9600, B9600},
		{115200, B115200},
		{57600, B57600},
		{1200, B1200},
		{2400, B2400},
		{19200, B19200},
		{38400, B38400},
	};

	void usage(const char *app) {
		std::cerr << "Usage:\n\n" << app << " options\n\n";
		std::cerr << "-m\n--master\n\tConfigures the XBee as a master; i.e. the modem attached to the computer.\n\n";
		std::cerr << "-s\n--slave\n\tConfigures the XBee as a slave; i.e. a modem in a robot.\n\n";
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

	bool read_messy_ok(int fd) {
		timespec end_time, cur_time;
		if (clock_gettime(CLOCK_MONOTONIC, &end_time) < 0) {perror("clock_gettime"); std::exit(1);}
		end_time.tv_nsec += 750000000;
		if (end_time.tv_nsec >= 1000000000) {
			end_time.tv_sec++;
			end_time.tv_nsec -= 1000000000;
		}

		bool seen_O = false;
		for (;;) {
			char ch;
			int rc;

			if (clock_gettime(CLOCK_MONOTONIC, &cur_time) < 0) {perror("clock_gettime"); std::exit(1);}
			if (cur_time.tv_sec > end_time.tv_sec || (cur_time.tv_sec == end_time.tv_sec && cur_time.tv_nsec > end_time.tv_nsec)) return false;
			rc = read(fd, &ch, 1);
			if (rc < 0) {perror("read"); std::exit(1);}
			else if (rc == 1) {
				if (seen_O && ch == 'K') return true;
				else if (ch == 'O') seen_O = true;
				else seen_O = false;
			}
		}
	}

	std::string read_line(int fd) {
		timespec end_time, cur_time;
		if (clock_gettime(CLOCK_MONOTONIC, &end_time) < 0) {perror("clock_gettime"); std::exit(1);}
		end_time.tv_nsec += 750000000;
		if (end_time.tv_nsec >= 1000000000) {
			end_time.tv_sec++;
			end_time.tv_nsec -= 1000000000;
		}

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
	unsigned int do_master = 0, do_slave = 0, do_help = 0;

	int ch;
	while ((ch = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, 0)) != -1)
		switch (ch) {
			case 'm':
				do_master++;
				break;

			case 's':
				do_slave++;
				break;

			default:
				do_help++;
				break;
		}

	if (do_help || do_master + do_slave + do_help != 1 || optind < argc - 1) {
		usage(argv[0]);
		return 1;
	}

	const char *path = "/dev/xbee";
	if (optind < argc) {
		path = argv[optind];
	}

	int fd = open(path, O_RDWR);
	if (fd < 0) {perror("open"); return 1;}

start_work:
	for (unsigned int baudidx = 0; baudidx < sizeof(BAUDS) / sizeof(*BAUDS); baudidx++) {
		for (unsigned int retries = 0; retries < 3; retries++) {
			std::cout << "Entering command mode at " << BAUDS[baudidx].display << " baud... " << std::flush;

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

			send_fully(fd, "+++", 3);

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
	send_fully(fd, "ATVR\r", 5);
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
	send_fully(fd, "ATRE\r", 5);
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting API mode with escapes... " << std::flush;
	send_fully(fd, "ATAP2\r", 6);
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting serial baud rate... " << std::flush;
	send_fully(fd, do_master ? "ATBD7\r" : "ATBD6\r", 6);
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting radio channel... " << std::flush;
	send_fully(fd, "ATCHE\r", 6);
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	if (do_slave) {
		std::cout << "Setting DIO0 to default as digital output high... " << std::flush;
		send_fully(fd, "ATD05\r", 6);
		if (read_clean_ok(fd)) {
			std::cout << "OK\n";
		} else {
			std::cout << "FAIL\n";
			goto start_work;
		}
	}

	std::cout << "Setting PAN ID... " << std::flush;
	send_fully(fd, "ATID6789\r", 9);
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Setting 16-bit address to none... " << std::flush;
	send_fully(fd, "ATMYFFFF\r", 9);
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Saving configuration... " << std::flush;
	send_fully(fd, "ATWR\r", 5);
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\n";
		goto start_work;
	}

	std::cout << "Rebooting modem... " << std::flush;
	send_fully(fd, "ATFR\r", 5);
	if (read_clean_ok(fd)) {
		std::cout << "OK\n";
	} else {
		std::cout << "FAIL\nThe modem did not reboot. Please power cycle it.\n";
	}

	return 0;
}

