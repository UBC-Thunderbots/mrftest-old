#define DEBUG 0
#define FAKEPORT 0
#include "util/dprint.h"
#include "xbee/daemon/physical/serial.h"
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
#if !FAKEPORT
	bool is_serinfo_ok(const serial_struct &serinfo) {
		if ((serinfo.flags & ASYNC_SPD_MASK) != ASYNC_SPD_CUST) {
			DPRINT(Glib::ustring::compose("Serial: want speed mask %1, have %2", ASYNC_SPD_CUST, serinfo.flags & ASYNC_SPD_MASK));
			return false;
		}
		if (serinfo.baud_base / serinfo.custom_divisor != 250000) {
			DPRINT(Glib::ustring::compose("Serial: want baud rate 250000, have crystal speed %1, divisor %2, baud %3", serinfo.baud_base, serinfo.custom_divisor, serinfo.baud_base / serinfo.custom_divisor));
			return false;
		}
		return true;
	}

	bool is_tios_ok(const termios &tios) {
		if ((tios.c_iflag & (IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IGNPAR | INPCK | IXOFF)) != IGNPAR) {
			DPRINT("Serial: iflag needs changing");
			return false;
		}
		if ((tios.c_oflag & (OPOST | OCRNL | ONOCR | ONLRET | OFILL | NLDLY | CRDLY | TABDLY | BSDLY | VTDLY | FFDLY | NL0 | CR0 | TAB0 | BS0 | VT0 | FF0)) != (NL0 | CR0 | TAB0 | BS0 | VT0 | FF0)) {
			DPRINT("Serial: oflag needs changing");
			return false;
		}
		if ((tios.c_cflag & (CSIZE | PARENB | CS8 | CREAD | CRTSCTS | CSTOPB | CLOCAL)) != (CS8 | CREAD | CRTSCTS)) {
			DPRINT("Serial: cflag needs changing");
			return false;
		}
		if ((tios.c_lflag & (ECHO | ECHONL | ICANON | ISIG | IEXTEN)) != 0) {
			DPRINT("Serial: lflag needs changing");
			return false;
		}
		if (tios.c_cc[VMIN] != 0) {
			DPRINT("Serial: c_cc[VMIN] needs changing");
			return false;
		}
		if (tios.c_cc[VTIME] != 0) {
			DPRINT("Serial: c_cc[VTIME] needs changing");
			return false;
		}
		return true;
	}
#endif
}

serial_port::serial_port() : port("/dev/xbee", O_RDWR | O_NOCTTY) {
}

void serial_port::configure_port() {
#if !FAKEPORT
	// Try to configure the custom divisor to give 250,000 baud.
	serial_struct cur_serinfo, new_serinfo;
	if (ioctl(port, TIOCGSERIAL, &cur_serinfo) < 0)
		throw std::runtime_error("Cannot get serial port configuration!");
	new_serinfo = cur_serinfo;
	new_serinfo.flags &= ~ASYNC_SPD_MASK;
	new_serinfo.flags |= ASYNC_SPD_CUST;
	new_serinfo.custom_divisor = new_serinfo.baud_base / 250000;
	while (!is_serinfo_ok(cur_serinfo)) {
		if (ioctl(port, TIOCSSERIAL, &new_serinfo) < 0)
			throw std::runtime_error("Cannot set serial port configuration!");
		if (ioctl(port, TIOCGSERIAL, &cur_serinfo) < 0)
			throw std::runtime_error("Cannot get serial port configuration!");
	}

	// Try to configure the regular terminal to 38,400 baud (which means to use the custom divisor).
	termios cur_tios, new_tios;
	if (tcgetattr(port, &cur_tios) < 0)
		throw std::runtime_error("Cannot get serial port configuration!");
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
		if (tcsetattr(port, TCSAFLUSH, &new_tios) < 0)
			throw std::runtime_error("Cannot set serial port configuration!");
		if (tcgetattr(port, &cur_tios) < 0)
			throw std::runtime_error("Cannot get serial port configuration!");
	}
#endif

	// Register for readability notifications.
	Glib::signal_io().connect(sigc::mem_fun(this, &serial_port::on_readable), port, Glib::IO_IN);
}

void serial_port::send(iovec *iov, std::size_t iovcnt) {
#if DEBUG
	Glib::ustring msg("TX:");
	for (int i = 0; i < iovcnt; ++i) {
		for (unsigned int j = 0; j < iov[i].iov_len; ++j) {
			msg.push_back(' ');
			msg.append(tohex(static_cast<const unsigned char *>(iov[i].iov_base)[j], 2));
		}
	}
	DPRINT(msg);
#endif

	while (iovcnt) {
		ssize_t written = writev(port, iov, static_cast<int>(iovcnt));
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

bool serial_port::on_readable(Glib::IOCondition) {
	uint8_t buffer[256];
	ssize_t ret = read(port, buffer, sizeof(buffer));
	if (ret < 0)
		throw std::runtime_error("Cannot read from serial port!");

#if DEBUG
	Glib::ustring msg("RX:");
	for (int i = 0; i < ret; i++) {
		msg.push_back(' ');
		msg.append(tohex(buffer[i], 2));
	}
	DPRINT(msg);
#endif

	sig_received.emit(buffer, ret);

	return true;
}

