#include "util/sockaddrs.h"
#include "xbee/xbee.h"
#include "xbee/daemon/daemon.h"
#include <stdexcept>
#include <algorithm>
#include <cerrno>
#include <glibmm.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace {
	bool connect_to_existing_daemon(file_descriptor &sock) {
		// Calculate path.
		const Glib::ustring &cache_dir = Glib::get_user_cache_dir();
		const std::string &socket_path = Glib::filename_from_utf8(cache_dir + "/thunderbots-xbeed-sock");

		// Create a socket.
		file_descriptor tmp(PF_UNIX, SOCK_SEQPACKET, 0);
		sockaddrs sa;
		sa.un.sun_family = AF_UNIX;
		if (socket_path.size() > sizeof(sa.un.sun_path)) throw std::runtime_error("Path too long!");
		std::copy(socket_path.begin(), socket_path.end(), &sa.un.sun_path[0]);
		std::fill(&sa.un.sun_path[socket_path.size()], &sa.un.sun_path[sizeof(sa.un.sun_path) / sizeof(*sa.un.sun_path)], '\0');
		if (connect(tmp, &sa.sa, sizeof(sa)) < 0) {
			if (errno == ECONNREFUSED) return false;
			throw std::runtime_error("Cannot connect to arbiter daemon!");
		}

		// Read the signature string from the daemon. If this fails, we may have
		// hit the race condition when the daemon is dying.
		char buffer[4];
		if (recv(tmp, buffer, sizeof(buffer), 0) != sizeof(buffer))
			return false;
		if (buffer[0] != 'X' || buffer[1] != 'B' || buffer[2] != 'E' || buffer[3] != 'E')
			return false;

		// Return the socket.
		sock = tmp;
		return true;
	}

	file_descriptor connect_to_daemon() {
		file_descriptor sock;

		// Loop forever, until something works.
		for (;;) {
			// Try running the daemon ourself.
			if (xbeedaemon::launch(sock))
				return sock;

			// Try connecting to an already-running daemon.
			if (connect_to_existing_daemon(sock))
				return sock;
		}
	}
}

xbee::xbee() : sock(connect_to_daemon()) {
	Glib::signal_io().connect(sigc::mem_fun(*this, &xbee::on_readable), sock, Glib::IO_IN);
}

void xbee::send(const void *data, std::size_t length) {
	ssize_t ret = ::send(sock, data, length, MSG_EOR | MSG_NOSIGNAL);
	if (ret < 0)
		throw std::runtime_error("Cannot talk to XBee arbiter!");
	if (ret < static_cast<ssize_t>(length))
		throw std::runtime_error("Message truncated talking to XBee arbiter!");
}

void xbee::unlock() {
	char ch;
	send(&ch, 0);
}

bool xbee::on_readable(Glib::IOCondition) {
	char buffer[65536];
	ssize_t ret = recv(sock, buffer, sizeof(buffer), 0);
	if (ret < 0)
		throw std::runtime_error("Cannot talk to XBee arbiter!");
	if (!ret)
		throw std::runtime_error("XBee arbiter died!");
	sig_received.emit(buffer, ret);
	return true;
}

