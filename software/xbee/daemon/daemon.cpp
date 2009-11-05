#include "util/args.h"
#include "util/sockaddrs.h"
#include "xbee/daemon/daemon.h"
#include "xbee/daemon/packetproto.h"
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <cerrno>
#include <ctime>
#include <glibmm.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <poll.h>

namespace {
	class client_service_provider : public noncopyable, public sigc::trackable {
		public:
			client_service_provider(int sock, xbee_packet_stream &pstream) : sock(sock), pstream(pstream) {
			}

			void run() {
				// Set up poll structures for the XBee and the client.
				pollfd pfds[2];
				pfds[0].fd = pstream.fd();
				pfds[0].events = POLLIN;
				pfds[0].revents = 0;
				pfds[1].fd = sock;
				pfds[1].events = POLLIN;
				pfds[1].revents = 0;

				// Attach to the XBee for incoming packets.
				pstream.signal_received().connect(sigc::mem_fun(*this, &client_service_provider::xbee_receive));

				// Go into a loop.
				for (;;) {
					// Receive a packet from the client.
					uint8_t buffer[65536];
					ssize_t len = recv(sock, buffer, sizeof(buffer), 0);
					if (len <= 0)
						return;

					// Send the packet to the XBee.
					pstream.send(buffer, len);
				}
			}

		private:
			int sock;
			xbee_packet_stream &pstream;

			void xbee_receive(const std::vector<uint8_t> &pkt) {
				// Send the packet over the UNIX socket.
				send(sock, &pkt[0], pkt.size(), MSG_EOR | MSG_NOSIGNAL);
			}
	};

	void run_daemon_impl(const file_descriptor &listen_sock, xbee_packet_stream &pstream) {
		// Prepare a vector of pollfds to watch.
		// Initially, we have only the serial port and the listening socket.
		std::vector<pollfd> pfds(2);
		pfds[0].fd = pstream.fd();
		pfds[0].events = POLLIN;
		pfds[0].revents = 0;
		pfds[1].fd = listen_sock;
		pfds[1].events = POLLIN;
		pfds[1].revents = 0;

		// Loop until nobody is connected to us.
		// Special case: iterate the loop once on startup because the initial parent
		// process who forked us is pending in the connect queue. Hence the "do"
		// rather than plain "while".
		do {
			// Poll the descriptor set.
			if (poll(&pfds[0], pfds.size(), -1) < 0)
				throw std::runtime_error("Cannot poll!");

			// First check for activity on the serial port.
			if (pfds[0].revents & (POLLHUP | POLLERR | POLLNVAL))
				throw std::runtime_error("Serial port failure!");
			if (pfds[0].revents & POLLIN)
				pstream.readable();

			// Next check for a connection from a new client.
			if (pfds[1].revents & (POLLHUP | POLLERR | POLLNVAL))
				throw std::runtime_error("Listening UNIX-domain socket failure!");
			if (pfds[1].revents & POLLIN) {
				int fd = accept(listen_sock, 0, 0);
				if (fd >= 0) {
					if (send(fd, "XBEE", 4, MSG_NOSIGNAL | MSG_EOR) == 4) {
						pollfd pfd;
						pfd.fd = fd;
						pfd.events = POLLIN;
						pfd.revents = 0;
						pfds.push_back(pfd);
					} else {
						close(fd);
					}
				}
			}

			// Check for disconnection or failure of clients.
			for (std::vector<pollfd>::size_type i = 2; i < pfds.size(); i++)
				if (pfds[i].events & (POLLHUP | POLLERR | POLLNVAL)) {
					close(pfds[i].fd);
					pfds.erase(pfds.begin() + i);
					i--;
				}

			// Make a list of all the clients who have sent us packets and hence
			// are awaiting service.
			std::vector<int> ready;
			for (std::vector<pollfd>::const_iterator i = pfds.begin(), iend = pfds.end(); i != iend; ++i)
				if (i->events & POLLIN)
					ready.push_back(i->fd);

			// Pick a random client and service it.
			client_service_provider serv(ready[std::rand() % ready.size()], pstream);
			serv.run();
		} while (pfds.size() > 2);
	}

	__attribute__((noreturn)) void run_daemon(const file_descriptor &listen_sock, xbee_packet_stream &pstream);
	void run_daemon(const file_descriptor &listen_sock, xbee_packet_stream &pstream) {
		try {
			run_daemon_impl(listen_sock, pstream);
			_exit(0);
		} catch (...) {
		}
		_exit(1);
	}
}

namespace xbeedaemon {
	bool launch(file_descriptor &client_sock) {
		// Calculate filenames.
		const Glib::ustring &cache_dir = Glib::get_user_cache_dir();
		const std::string &lock_path = Glib::filename_from_utf8(cache_dir + "/thunderbots-xbeed-lock");
		const std::string &socket_path = Glib::filename_from_utf8(cache_dir + "/thunderbots-xbeed-sock");

		// Check if we can lock the lock file.
		const file_descriptor lock_file(lock_path.c_str(), O_CREAT | O_RDWR);
		if (flock(lock_file, LOCK_EX | LOCK_NB) < 0) {
			if (errno != EWOULDBLOCK && errno != EAGAIN)
				throw std::runtime_error("Cannot lock lock file!");
			return false;
		}

		// Delete any existing socket.
		unlink(socket_path.c_str());

		// Create and bind the listening socket.
		const file_descriptor listen_sock(PF_UNIX, SOCK_SEQPACKET, 0);
		listen_sock.set_blocking(false);
		sockaddrs sa;
		sa.un.sun_family = AF_UNIX;
		if (socket_path.size() > sizeof(sa.un.sun_path)) throw std::runtime_error("Path too long!");
		std::copy(socket_path.begin(), socket_path.end(), &sa.un.sun_path[0]);
		std::fill(&sa.un.sun_path[socket_path.size()], &sa.un.sun_path[sizeof(sa.un.sun_path) / sizeof(*sa.un.sun_path)], '\0');
		if (bind(listen_sock, &sa.sa, sizeof(sa)) < 0) throw std::runtime_error("Cannot bind UNIX-domain socket!");
		if (listen(listen_sock, SOMAXCONN) < 0) throw std::runtime_error("Cannot listen on UNIX-domain socket!");

		// Connect another socket to it.
		const file_descriptor temp_client_sock(PF_UNIX, SOCK_SEQPACKET, 0);
		if (connect(temp_client_sock, &sa.sa, sizeof(sa)) < 0) throw std::runtime_error("Cannot connect to UNIX-domain socket!");
		temp_client_sock.set_blocking(false);

		// Open the serial port.
		xbee_packet_stream pstream;

		// Seed the random number generator.
		std::srand(std::time(0));

		// Fork a process.
		pid_t child_pid = fork();
		if (child_pid < 0) {
			// Fork failed.
			throw std::runtime_error("Cannot fork!");
		} else if (child_pid > 0) {
			// This is the parent process.
			// The child will hold the lock file and listen socket, so we can close them.
			// We will keep the client socket, and the child will close it.
			client_sock = temp_client_sock;
			return true;
		} else {
			// This is the child process.
			// We need to keep the lock file, listen socket, and serial port.
			// Close all the others.
			for (int i = 0; i < 65536; i++)
				if (i != listen_sock && i != lock_file && i != pstream.fd())
					close(i);
			// Enter a new session and process group.
			setsid();
			// Display the proper name.
			static const char PROCESS_NAME[] = "xbeed";
			prctl(PR_SET_NAME, PROCESS_NAME, 0, 0, 0);
			std::copy(PROCESS_NAME, PROCESS_NAME + sizeof(PROCESS_NAME), args::argv[0]);
			// Run as the multiplexing daemon.
			run_daemon(listen_sock, pstream);
		}
	}
}

