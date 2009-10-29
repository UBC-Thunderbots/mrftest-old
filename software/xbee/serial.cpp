#include "util/sockaddrs.h"
#include "xbee/serial.h"
#include "xbee/xbeed.h"
#include <stdexcept>
#include <string>
#include <algorithm>
#include <cerrno>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <asm/ioctls.h>
#include <linux/serial.h>

namespace {
	void set_nonblocking(int fd) {
		long flags = fcntl(fd, F_GETFL);
		if (flags < 0) throw std::runtime_error("Cannot get file flags!");
		flags |= O_NONBLOCK;
		if (fcntl(fd, F_SETFL, flags) < 0) throw std::runtime_error("Cannot set file flags!");
	}

	file_descriptor open_file(const char *path) {
		int fd = open(path, O_RDWR | O_NOCTTY);
		if (fd < 0)
			throw std::runtime_error("Cannot open file!");
		file_descriptor obj(fd);
		return obj;
	}

	void configure_serial_port(file_descriptor &fd) {
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
		tios.c_cflag |= CSTOPB | CREAD | CRTSCTS;
		tios.c_cflag &= ~CLOCAL;
		tios.c_cc[VMIN] = 0;
		tios.c_cc[VTIME] = 0;
		if (tcsetattr(fd, TCSAFLUSH, &tios) < 0)
			throw std::runtime_error("Cannot set serial port configuration!");
	}

	file_descriptor open_port() {
		file_descriptor fd(open_file("/dev/xbee"));
		configure_serial_port(fd);
		set_nonblocking(fd);
		return fd;
	}

	file_descriptor create_unix_socket() {
		int fd = socket(PF_UNIX, SOCK_STREAM, 0);
		if (fd < 0)
			throw std::runtime_error("Cannot create UNIX socket!");
		file_descriptor obj(fd);
		return obj;
	}

	file_descriptor open_socket() {
		sockaddrs sa;

		// Determine the paths to the lock file and socket.
		const Glib::ustring &cache_dir = Glib::get_user_cache_dir();
		const std::string &lock_path = Glib::filename_from_utf8(cache_dir + "/thunderbots-xbeed-lock");
		const std::string &socket_path = Glib::filename_from_utf8(cache_dir + "/thunderbots-xbeed-sock");
		if (socket_path.size() > sizeof(sa.un.sun_path)) throw std::runtime_error("Path too long!");

		// Open the lock file.
		file_descriptor lock_file(open_file(lock_path.c_str()));

		// Create the socket.
		file_descriptor sock;

		for (;;) {
			// Try spawning the daemon.
			if (xbeed::launch(lock_file, sock))
				return sock;

			// Looks like the daemon is already running. Connect to it.
			sock = create_unix_socket();
			sockaddrs sa;
			sa.un.sun_family = AF_UNIX;
			std::copy(socket_path.begin(), socket_path.end(), sa.un.sun_path);
			std::fill(sa.un.sun_path + socket_path.size(), sa.un.sun_path + sizeof(sa.un.sun_path), '\0');
			if (connect(sock, &sa.sa, sizeof(sa)) < 0) throw std::runtime_error("Cannot connect UNIX-domain socket!");

			// Ping it.
			uint8_t buf = xbeed::REQUEST_PING;
			ssize_t ret = send(sock, &buf, 1, MSG_NOSIGNAL);
			if (ret < 0) {
				if (errno == ECONNRESET) continue;
				throw std::runtime_error("Cannot ping daemon!");
			}

			// Wait for response.
			ret = recv(sock, &buf, 1, 0);
			if (ret < 0) throw std::runtime_error("Cannot read from UNIX-domain socket!");
			if (!ret) continue;
			if (buf != xbeed::RESPONSE_PONG) throw std::runtime_error("Protocol error on UNIX-domain socket!");

			// Win!
			return sock;
		}
	}
}

serial_port::serial_port() : sock(open_socket()), port(open_port()), locked(false) {
	// Set up callbacks for incoming data.
	Glib::signal_io().connect(sigc::mem_fun(*this, &serial_port::on_port_readable), port, Glib::IO_IN);
	Glib::signal_io().connect(sigc::mem_fun(*this, &serial_port::on_sock_readable), sock, Glib::IO_IN);
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

void serial_port::lock(const sigc::slot<void> &on_acquire, bool batch) {
	if (locked) {
		// No need to request; unlock() will hand the lock to next requester.
	} else if (!lock_callbacks.empty()) {
		// No need to request; the last lock() call requested.
	} else {
		// Need to send the request.
		uint8_t req = batch ? xbeed::REQUEST_LOCK_BATCH : xbeed::REQUEST_LOCK_INTERACTIVE;
		ssize_t ret = ::send(sock, &req, 1, MSG_NOSIGNAL);
		if (ret < 0) throw std::runtime_error("Cannot send request on UNIX-domain socket!");
		if (!ret) throw std::runtime_error("Overflow on UNIX-domain socket!");
	}
	lock_callbacks.push(on_acquire);
}

void serial_port::unlock() {
	if (lock_callbacks.empty()) {
		uint8_t req = xbeed::REQUEST_UNLOCK;
		ssize_t ret = ::send(sock, &req, 1, MSG_NOSIGNAL);
		if (ret < 0) throw std::runtime_error("Cannot send request on UNIX-domain socket!");
		if (!ret) throw std::runtime_error("Overflow on UNIX-domain socket!");
		locked = false;
	} else {
		sigc::slot<void> cb = lock_callbacks.front();
		lock_callbacks.pop();
		cb();
	}
}

bool serial_port::on_port_readable(Glib::IOCondition) {
	uint8_t buffer[32];
	ssize_t ret = read(port, buffer, sizeof(buffer));
	if (ret < 0) {
		throw std::runtime_error("Cannot read from serial port!");
	}
	for (int i = 0; i < ret; i++) {
		sig_received.emit(buffer[i]);
	}
	return true;
}

bool serial_port::on_sock_readable(Glib::IOCondition) {
	uint8_t resp;
	ssize_t ret = recv(sock, &resp, 1, 0);
	if (ret < 0) throw std::runtime_error("Cannot read from UNIX-domain socket!");
	else if (!ret) throw std::runtime_error("Lock manager daemon died!");
	if (resp == xbeed::RESPONSE_GRANT) {
		assert(!locked);
		assert(!lock_callbacks.empty());
		locked = true;
		sigc::slot<void> cb = lock_callbacks.front();
		lock_callbacks.pop();
		cb();
	} else {
		throw std::runtime_error("Protocol error on UNIX-domain socket!");
	}
	return true;
}

