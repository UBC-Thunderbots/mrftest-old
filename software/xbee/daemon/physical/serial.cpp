#include "xbee/daemon/physical/serial.h"
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <asm/ioctls.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	bool is_serinfo_ok(const serial_struct &serinfo) {
		if ((serinfo.flags & ASYNC_SPD_MASK) != ASYNC_SPD_CUST) {
			return false;
		}
		if (serinfo.baud_base / serinfo.custom_divisor != 250000) {
			return false;
		}
		return true;
	}

	bool is_tios_ok(const termios &tios) {
		if ((tios.c_iflag & (IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IGNPAR | INPCK | IXOFF)) != IGNPAR) {
			return false;
		}
		if ((tios.c_oflag & (OPOST | OCRNL | ONOCR | ONLRET | OFILL | NLDLY | CRDLY | TABDLY | BSDLY | VTDLY | FFDLY | NL0 | CR0 | TAB0 | BS0 | VT0 | FF0)) != (NL0 | CR0 | TAB0 | BS0 | VT0 | FF0)) {
			return false;
		}
		if ((tios.c_cflag & (CSIZE | PARENB | CS8 | CREAD | CRTSCTS | CSTOPB | CLOCAL)) != (CS8 | CREAD | CRTSCTS)) {
			return false;
		}
		if ((tios.c_lflag & (ECHO | ECHONL | ICANON | ISIG | IEXTEN)) != 0) {
			return false;
		}
		if (tios.c_cc[VMIN] != 0) {
			return false;
		}
		if (tios.c_cc[VTIME] != 0) {
			return false;
		}
		return true;
	}
}

SerialPort::SerialPort() : port(FileDescriptor::create_open("/dev/xbee", O_RDWR | O_NOCTTY, 0)) {
}

void SerialPort::configure_port() {
	// Try to configure the custom divisor to give 250,000 baud.
	serial_struct cur_serinfo, new_serinfo;
	if (ioctl(port->fd(), TIOCGSERIAL, &cur_serinfo) < 0) {
		throw std::runtime_error("Cannot get serial port configuration!");
	}
	new_serinfo = cur_serinfo;
	new_serinfo.flags &= ~ASYNC_SPD_MASK;
	new_serinfo.flags |= ASYNC_SPD_CUST;
	new_serinfo.custom_divisor = new_serinfo.baud_base / 250000;
	while (!is_serinfo_ok(cur_serinfo)) {
		if (ioctl(port->fd(), TIOCSSERIAL, &new_serinfo) < 0) {
			throw std::runtime_error("Cannot set serial port configuration!");
		}
		if (ioctl(port->fd(), TIOCGSERIAL, &cur_serinfo) < 0) {
			throw std::runtime_error("Cannot get serial port configuration!");
		}
	}

	// Try to configure the regular terminal to 38,400 baud (which means to use the custom divisor).
	termios cur_tios, new_tios;
	if (tcgetattr(port->fd(), &cur_tios) < 0) {
		throw std::runtime_error("Cannot get serial port configuration!");
	}
	new_tios = cur_tios;
	cfmakeraw(&new_tios);
	cfsetispeed(&new_tios, B38400);
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
		if (tcsetattr(port->fd(), TCSAFLUSH, &new_tios) < 0) {
			throw std::runtime_error("Cannot set serial port configuration!");
		}
		if (tcgetattr(port->fd(), &cur_tios) < 0) {
			throw std::runtime_error("Cannot get serial port configuration!");
		}
	}

	// Register for readability notifications.
	Glib::signal_io().connect(sigc::mem_fun(this, &SerialPort::on_readable), port->fd(), Glib::IO_IN);
}

void SerialPort::send(iovec *iov, std::size_t iovcnt) {
	while (iovcnt) {
		ssize_t written = writev(port->fd(), iov, static_cast<int>(iovcnt));
		if (written < 0) {
			throw std::runtime_error("Cannot write to serial port!");
		}
		while (written && iovcnt) {
			std::size_t s = static_cast<std::size_t>(written) < iov->iov_len ? static_cast<std::size_t>(written) : iov->iov_len;
			iov->iov_base = static_cast<uint8_t *>(iov->iov_base) + s;
			iov->iov_len -= s;
			written -= s;
			if (!iov->iov_len) {
				++iov;
				--iovcnt;
			}
		}
		if (written && !iovcnt) {
			throw std::runtime_error("Wrote more data than existed!");
		}
	}
}

bool SerialPort::on_readable(Glib::IOCondition) {
	uint8_t buffer[256];
	ssize_t ret = read(port->fd(), buffer, sizeof(buffer));
	if (ret < 0) {
		throw std::runtime_error("Cannot read from serial port!");
	}

	sig_received.emit(buffer, ret);

	return true;
}

