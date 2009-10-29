#include "util/args.h"
#include "util/fd.h"
#include "util/sockaddrs.h"
#include "xbee/xbeed.h"
#include <stdexcept>
#include <string>
#include <list>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cerrno>
#include <glibmm.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

namespace {
	int create_unix_socket() {
		int fd = socket(PF_INET, SOCK_STREAM, 0);
		if (fd < 0)
			throw std::runtime_error("Cannot create UNIX socket!");
		return fd;
	}

	void set_nonblocking(int fd) {
		long flags = fcntl(fd, F_GETFL);
		if (flags < 0) throw std::runtime_error("Cannot get file flags!");
		flags |= O_NONBLOCK;
		if (fcntl(fd, F_SETFL, flags) < 0) throw std::runtime_error("Cannot set file flags!");
	}

	int create_epoll() {
		int fd = epoll_create1(0);
		if (fd < 0)
			throw std::runtime_error("Cannot create epoll descriptor!");
		return fd;
	}

	void push_grants(std::list<int> &requests, int &lock_held_by) {
		while (lock_held_by == -1 && !requests.empty()) {
			int next = requests.front();
			requests.pop_front();
			uint8_t resp = xbeed::RESPONSE_GRANT;
			if (send(next, &resp, 1, MSG_NOSIGNAL) == 1)
				lock_held_by = next;
		}
	}

	void run_daemon_impl(const file_descriptor &listen_sock) {
		const file_descriptor epollfd(create_epoll());
		epoll_event events[8];
		events[0].events = EPOLLIN;
		events[0].data.fd = listen_sock;
		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &events[0]) < 0) throw std::runtime_error("Cannot register with epoll!");
		unsigned int num_connected = 0;
		int lock_held_by = -1;
		std::list<int> requests;

		do {
			int ret = epoll_wait(epollfd, events, sizeof(events) / sizeof(*events), num_connected ? -1 : 500);
			if (ret < 0) throw std::runtime_error("Cannot epoll!");
			for (int i = 0; i < ret; i++) {
				if (events[i].data.fd == listen_sock) {
					if (events[i].events & EPOLLERR)
						throw std::runtime_error("Error on listen socket!");
					else if (events[i].events & EPOLLHUP)
						throw std::runtime_error("Hangup on listen socket!");
					else if (events[i].events & EPOLLIN) {
						int fd = accept(listen_sock, 0, 0);
						if (fd >= 0) {
							events[0].events = EPOLLIN;
							events[0].data.fd = fd;
							if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &events[0]) < 0) throw std::runtime_error("Cannot register with epoll!");
							num_connected++;
						}
					} else
						throw std::runtime_error("Unrecognized event type!");
				} else {
					if (events[i].events & (EPOLLERR | EPOLLHUP)) {
						if (lock_held_by == events[i].data.fd)
							lock_held_by = -1;
						for (std::list<int>::iterator j = requests.begin(), jend = requests.end(); j != jend; ++j)
							if (*j == events[i].data.fd) {
								requests.erase(j);
								break;
							}
						if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, 0) < 0) throw std::runtime_error("Cannot unregister with epoll!");
						close(events[i].data.fd);
						num_connected--;
						push_grants(requests, lock_held_by);
					} else if (events[i].events & EPOLLIN) {
						uint8_t req;
						ssize_t ret = recv(events[i].data.fd, &req, 1, 0);
						if (ret < 0) throw std::runtime_error("Cannot read from UNIX-domain socket!");
						if (!ret) continue;
						if (req == xbeed::REQUEST_PING) {
							uint8_t resp = xbeed::RESPONSE_PONG;
							send(events[i].data.fd, &resp, 1, MSG_NOSIGNAL);
						} else if (req == xbeed::REQUEST_LOCK_INTERACTIVE || req == xbeed::REQUEST_LOCK_BATCH) {
							requests.push_back(events[i].data.fd);
							push_grants(requests, lock_held_by);
						} else if (req == xbeed::REQUEST_UNLOCK) {
							lock_held_by = -1;
							push_grants(requests, lock_held_by);
						}
					} else
						throw std::runtime_error("Unrecognized event type!");
				}
			}
		} while (num_connected);
	}

	__attribute__((noreturn)) void run_daemon(const file_descriptor &listen_sock);
	void run_daemon(const file_descriptor &listen_sock) {
		try {
			run_daemon_impl(listen_sock);
			_exit(0);
		} catch (...) {
		}
		_exit(1);
	}
}

namespace xbeed {
	bool launch(const file_descriptor &lock_fd, file_descriptor &client_sock) {
		// Try locking the lock file.
		if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
			if (errno != EWOULDBLOCK && errno != EAGAIN)
				throw std::runtime_error("Cannot lock lock file!");
			return false;
		}

		// Determine the path to the socket.
		const Glib::ustring &cache_dir = Glib::get_user_cache_dir();
		const std::string &socket_path = Glib::filename_from_utf8(cache_dir + "/thunderbots-xbeed-sock");

		// Delete any existing socket.
		unlink(socket_path.c_str());

		// Establish the listening socket.
		const file_descriptor listen_sock(create_unix_socket());
		set_nonblocking(listen_sock);

		// Bind the listening socket to its name.
		sockaddrs sa;
		sa.un.sun_family = AF_UNIX;
		if (socket_path.size() > sizeof(sa.un.sun_path)) throw std::runtime_error("Path too long!");
		std::copy(socket_path.begin(), socket_path.end(), sa.un.sun_path);
		std::fill(sa.un.sun_path + socket_path.size(), sa.un.sun_path + sizeof(sa.un.sun_path), '\0');
		if (bind(listen_sock, &sa.sa, sizeof(sa)) < 0) throw std::runtime_error("Cannot bind UNIX-domain socket!");

		// Start it listening.
		if (listen(listen_sock, SOMAXCONN) < 0) throw std::runtime_error("Cannot listen on UNIX-domain socket!");

		// Connect another socket to it.
		file_descriptor temp_client_sock(create_unix_socket());
		if (connect(temp_client_sock, &sa.sa, sizeof(sa)) < 0) throw std::runtime_error("Cannot connect to UNIX-domain socket!");
		set_nonblocking(temp_client_sock);

		// Fork a process.
		pid_t child_pid = fork();
		if (child_pid < 0) {
			// Fork failed.
			throw std::runtime_error("Cannot fork!");
		} else if (child_pid > 0) {
			// This is the parent process.
			// The file descriptors will be held by the child process.
			// In the parent they will be closed by ~file_descriptor(), except for client_sock.
			// We're good to go.
			client_sock = temp_client_sock;
			return true;
		} else {
			// This is the child process.
			// Close every file descriptor except the ones we need to keep.
			for (int i = 0; i < 65536; i++)
				if (i != listen_sock && i != lock_fd)
					close(i);
			// Enter a new session and process group.
			setsid();
			// Display the proper name.
			prctl(PR_SET_NAME, "xbeed", 0, 0, 0);
			std::strcpy(args::argv[0], "xbeed");
			// Run as the multiplexing daemon.
			run_daemon(listen_sock);
		}
	}
}

