#include "xbee/serial.h"
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <asm/ioctls.h>

namespace {
	int open_port(const Glib::ustring &filename) {
		const std::string &fn = Glib::filename_from_utf8(filename);
		int fd = open(fn.c_str(), O_RDWR | O_NOCTTY);
		if (fd < 0) {
			throw std::runtime_error("Cannot open serial port!");
		}
		return fd;
	}
}

serial_port::serial_port(const Glib::ustring &filename) : fd(open_port(filename)) {
	// Try to configure the custom divisor to 96 (which gives 250,000 in an FTDI).
	serial_struct serinfo;
	if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0) {
		throw std::runtime_error("Cannot get serial port configuration!");
	}
	serinfo.flags &= ~ASYNC_SPD_MASK;
	serinfo.flags |= ASYNC_SPD_CUST;
	serinfo.custom_divisor = serinfo.baud_base / 250000;
	if (ioctl(fd, TIOCSSERIAL, &serinfo) < 0) {
		throw std::runtime_error("Cannot set serial port configuration!");
	}

	// Try to configure the regular terminal to 38400 baud (which uses the custom divisor).
	termios tios;
	if (tcgetattr(fd, &tios) < 0) {
		throw std::runtime_error("Cannot get serial port configuration!");
	}
	cfmakeraw(&tios);
	cfsetispeed(&tios, B0);
	cfsetospeed(&tios, B38400);
	tios.c_iflag |= IGNPAR;
	tios.c_iflag &= ~INPCK & ~IXOFF;
	tios.c_oflag &= ~OCRNL & ~ONOCR & ~ONLRET & ~OFILL & ~NLDLY & ~CRDLY & ~TABDLY & ~BSDLY & ~VTDLY & ~FFDLY;
	tios.c_oflag |= NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
	tios.c_cflag |= CSTOPB | CREAD | CRTSCTS;
	tios.c_cflag &= ~CLOCAL;
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 0;
	if (tcsetattr(fd, TCSAFLUSH, &tios) < 0) {
		throw std::runtime_error("Cannot set serial port configuration!");
	}

	// Set up a callback for incoming data.
	Glib::signal_io().connect(sigc::mem_fun(*this, &serial_port::on_readable), fd, Glib::IO_IN);
}

void serial_port::send(uint8_t ch) {
	send(&ch, 1);
}

void serial_port::send(const void *payload, std::size_t length) {
	const uint8_t *dptr = static_cast<const uint8_t *>(payload);
	while (length) {
		ssize_t written = write(fd, dptr, length);
		if (written < 0)
			throw std::runtime_error("Cannot write to serial port!");
		dptr += written;
		length -= written;
	}
}

bool serial_port::on_readable(Glib::IOCondition) {
	uint8_t buffer[32];
	ssize_t ret = read(fd, buffer, sizeof(buffer));
	if (ret < 0) {
		throw std::runtime_error("Cannot read from serial port!");
	}
	for (int i = 0; i < ret; i++) {
		sig_received.emit(buffer[i]);
	}
	return true;
}

