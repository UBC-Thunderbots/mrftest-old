#include "util/clocksource_master.h"
#include <algorithm>
#include <stdexcept>
#include <cerrno>
#include <glibmm.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace {
	union sockaddrs {
		sockaddr sa;
		sockaddr_un un;
	};

	file_descriptor create_listen_sock() {
		// Compute the filename of the clock distribution socket.
		const Glib::ustring &cache_dir = Glib::get_user_cache_dir();
		const std::string &clock_path = Glib::filename_from_utf8(cache_dir + "/thunderbots-sim-clock");

		// Remove any existing socket.
		unlink(clock_path.c_str());

		// Create a new socket, set it as non-blocking, and bind it.
		file_descriptor fd(AF_UNIX, SOCK_STREAM, 0);
		fd.set_blocking(false);
		sockaddrs sa;
		sa.un.sun_family = AF_UNIX;
		if (clock_path.size() > sizeof(sa.un.sun_path))
			throw new std::runtime_error("Path too long!");
		std::copy(clock_path.begin(), clock_path.end(), &sa.un.sun_path[0]);
		std::fill(&sa.un.sun_path[clock_path.size()], &sa.un.sun_path[sizeof(sa.un.sun_path) / sizeof(*sa.un.sun_path)], '\0');
		if (bind(fd, &sa.sa, sizeof(sa)) < 0)
			throw std::runtime_error("Cannot bind UNIX-domain socket!");
		if (listen(fd, SOMAXCONN) < 0)
			throw std::runtime_error("Cannot listen on UNIX-domain socket!");

		return fd;
	}
}

clocksource_master::clocksource_master() : listen_sock(create_listen_sock()) {
}

clocksource_master::~clocksource_master() {
	for (std::list<int>::const_iterator i = socks.begin(), iend = socks.end(); i != iend; ++i) {
		close(*i);
	}
	socks.clear();
}

void clocksource_master::tick() {
	// Accept any new incoming connections.
	int accepted;
	while ((accepted = accept(listen_sock, 0, 0)) >= 0) {
		long flags = fcntl(accepted, F_GETFL);
		if (flags < 0)
			throw std::runtime_error("Cannot get file descriptor flags!");
		flags &= ~O_NONBLOCK;
		if (fcntl(accepted, F_SETFL, flags) < 0)
			throw std::runtime_error("Cannot set file descriptor flags!");
		socks.push_back(accepted);
	}
	if (errno != EAGAIN && errno != EWOULDBLOCK) {
		throw std::runtime_error("Cannot accept incoming socket!");
	}

	// Send the clock ticks out over the sockets.
	unsigned char ch = 0;
	if (!socks.empty()) {
		std::list<int>::iterator i = socks.begin(), iend = socks.end();
		std::list<int>::iterator next = i;
		++next;
		while (i != iend) {
			if (send(*i, &ch, 1, MSG_NOSIGNAL) != 1) {
				socks.erase(i);
			}
			i = next;
			++next;
		}
	}

	// Wait for acknowledgements.
	if (!socks.empty()) {
		std::list<int>::iterator i = socks.begin(), iend = socks.end();
		std::list<int>::iterator next = i;
		++next;
		while (i != iend) {
			if (recv(*i, &ch, 1, 0) != 1) {
				socks.erase(i);
			}
			i = next;
			++next;
		}
	}
}

