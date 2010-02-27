#define DEBUG 0
#include "util/dprint.h"
#include "xbee/daemon/serial.h"
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <asm/ioctls.h>
#include <linux/serial.h>

namespace {
	bool is_serinfo_ok(const serial_struct &serinfo) {
		if ((serinfo.flags & ASYNC_SPD_MASK) != ASYNC_SPD_CUST)
			return false;
		if (serinfo.baud_base / serinfo.custom_diviser != 250000)
			return false;
		return true;
	}

	bool is_tios_ok(const termios &tios) {
		if ((tios.c_iflag & (IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IGNPAR | INPCK | IXOFF)) != IGNPAR)
			return false;
		if ((tios.c_oflag & (OPOST | OCRNL | ONOCR | ONLRET | OFILL | NLDLY | CRDLY | TABDLY | BSDLY | VTDLY | FFDLY | NL0 | CR0 | TAB0 | BS0 | VT0 | FF0)) != (NL0 | CR0 | TAB0 | BS0 | VT0 | FF0))
			return false;
		if ((tios.c_cflag & (CSIZE | PARENB | CS8 | CREAD | CRTSCTS | CSTOPB | CLOCAL)) != (CS8 | CREAD | CRTSCTS))
			return false;
		if ((tios.c_lflag & (ECHO | ECHONL | ICANON | ISIG | IEXTEN)) != 0)
			return false;
		if (tios.c_cc[VMIN] != 0)
			return false;
		if (tios.c_cc[VTIME] != 0)
			return false;
		if (cfgetospeed(&tios) != B38400)
			return false;
		if (cfgetispeed(&tios) != B0)
			return false;
		return true;
	}

	void configure_port(const file_descriptor &fd) {
		// Try to configure the custom divisor to give 250,000 baud.
		serial_struct cur_serinfo, new_serinfo;
		if (ioctl(fd, TIOCGSERIAL, &cur_serinfo) < 0)
			throw std::runtime_error("Cannot get serial port configuration!");
		new_serinfo = cur_serinfo;
		new_serinfo.flags &= ~ASYNC_SPD_MASK;
		new_serinfo.flags |= ASYNC_SPD_CUST;
		new_serinfo.custom_divisor = new_serinfo.baud_base / 250000;
		while (!is_serinfo_ok(cur_serinfo)) {
			if (ioctl(fd, TIOCSSERIAL, &new_serinfo) < 0)
				throw std::runtime_error("Cannot set serial port configuration!");
			if (ioctl(fd, TIOCGSERIAL, &cur_serinfo) < 0)
				throw std::runtime_error("Cannot get serial port configuration!");
		}

		// Try to configure the regular terminal to 38,400 baud (which means to use the custom divisor).
		termios cur_tios, new_tios;
		if (tcgetattr(fd, &cur_tios) < 0)
			throw std::runtime_error("Cannot get serial port configuration!");
		new_tios = cur_tios;
		cfmakeraw(&new_tios);
		cfsetispeed(&new_tios, B0);
		cfsetospeed(&new_tios, B38400);
		new_tios.c_iflag |= IGNPAR;
		new_tios.c_iflag &= ~INPCK & ~IXOFF;
		new_tios.c_oflag &= ~OCRNL & ~ONOCR & ~ONLRET & ~OFILL & ~NLDLY & ~CRDLY & ~TABDLY & ~BSDLY & ~VTDLY & ~FFDLY;
		new_tios.c_oflag |= NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
		new_tios.c_cflag |= CREAD | CRTSCTS;
		new_tios.c_cflag &= ~CSTOPB & ~CLOCAL;
		new_tios.c_cc[VMIN] = 0;
		new_tios.c_cc[VTIME] = 0;
		while (!is_tios_ok(cur_tios)) {
			if (tcsetattr(fd, TCSAFLUSH, &new_tios) < 0)
				throw std::runtime_error("Cannot set serial port configuration!");
			if (tcgetattr(fd, &cur_tios) < 0)
				throw std::runtime_error("Cannot get serial port configuration!");
		}
	}
}

serial_port::serial_port() : port("/dev/xbee", O_RDWR | O_NOCTTY) {
	// Configure the port.
	configure_port(port);
}

void serial_port::send(uint8_t ch) {
	send(&ch, 1);
}

void serial_port::send(const void *payload, std::size_t length) {
#if DEBUG
	Glib::ustring msg("TX:");
	for (unsigned int i = 0; i < length; i++) {
		msg.push_back(' ');
		msg.append(Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), static_cast<const unsigned char *>(payload)[i]));
	}
	DPRINT(msg);
#endif

	const uint8_t *dptr = static_cast<const uint8_t *>(payload);
	while (length) {
		ssize_t written = write(port, dptr, length);
		if (written < 0)
			throw std::runtime_error("Cannot write to serial port!");
		dptr += written;
		length -= written;
	}
}

void serial_port::readable() {
	uint8_t buffer[32];
	ssize_t ret = read(port, buffer, sizeof(buffer));
	if (ret < 0)
		throw std::runtime_error("Cannot read from serial port!");

#if DEBUG
	Glib::ustring msg("RX:");
	for (int i = 0; i < ret; i++) {
		msg.push_back(' ');
		msg.append(Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), buffer[i]));
	}
	DPRINT(msg);
#endif

	sig_received.emit(buffer, ret);
}

