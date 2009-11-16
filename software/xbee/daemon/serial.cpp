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
	void configure_port(const file_descriptor &fd) {
		// Try to configure the custom divisor to give 250,000 baud.
		serial_struct serinfo;
		if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0)
			throw std::runtime_error("Cannot get serial port configuration!");
		serinfo.flags &= ~ASYNC_SPD_MASK;
		serinfo.flags |= ASYNC_SPD_CUST;
		serinfo.custom_divisor = serinfo.baud_base / 250000;
		if (ioctl(fd, TIOCSSERIAL, &serinfo) < 0)
			throw std::runtime_error("Cannot set serial port configuration!");

		// Try to configure the regular terminal to 38,400 baud (which means to use the custom divisor).
		termios tios;
		if (tcgetattr(fd, &tios) < 0)
			throw std::runtime_error("Cannot get serial port configuration!");
		cfmakeraw(&tios);
		cfsetispeed(&tios, B0);
		cfsetospeed(&tios, B38400);
		tios.c_iflag |= IGNPAR;
		tios.c_iflag &= ~INPCK & ~IXOFF;
		tios.c_oflag &= ~OCRNL & ~ONOCR & ~ONLRET & ~OFILL & ~NLDLY & ~CRDLY & ~TABDLY & ~BSDLY & ~VTDLY & ~FFDLY;
		tios.c_oflag |= NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
		tios.c_cflag |= CREAD | CRTSCTS;
		tios.c_cflag &= ~CSTOPB & ~CLOCAL;
		tios.c_cc[VMIN] = 0;
		tios.c_cc[VTIME] = 0;
		if (tcsetattr(fd, TCSAFLUSH, &tios) < 0)
			throw std::runtime_error("Cannot set serial port configuration!");
	}
}

serial_port::serial_port() : port("/dev/xbee", O_RDWR | O_NOCTTY) {
	// Configure the port.
	configure_port(port);
	port.set_blocking(false);
}

void serial_port::send(uint8_t ch) {
	send(&ch, 1);
}

void serial_port::send(const void *payload, std::size_t length) {
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
	sig_received.emit(buffer, ret);
}

