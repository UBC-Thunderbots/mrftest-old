#define DEBUG 0
#include "util/args.h"
#include "util/dprint.h"
#include "util/sockaddrs.h"
#include "xbee/daemon/daemon.h"
#include "xbee/daemon/packetproto.h"
#include "xbee/util.h"
#include <stdexcept>
#include <exception>
#include <algorithm>
#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <cstdlib>
#include <cerrno>
#include <ctime>
#include <cstring>
#include <stdint.h>
#include <glibmm.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <sys/epoll.h>

#if DEBUG
#define FIRST_CLOSE_FD 3
#else
#define FIRST_CLOSE_FD 0
#endif

namespace {
	file_descriptor create_epollfd() {
		int fd = epoll_create(8);
		if (fd < 0) throw std::runtime_error("Cannot create epollfd!");
		return file_descriptor::create(fd);
	}

	struct client_info {
		// All global frame IDs that route to this client.
		std::tr1::unordered_set<uint8_t> frameid_reverse;
		// All XBee addresses that route to this client.
		std::tr1::unordered_set<uint64_t> address_reverse;
	};

	class daemon : public sigc::trackable {
		public:
			daemon(file_descriptor listen_sock, xbee_packet_stream &pstream);
			void run();

		private:
			file_descriptor listen_sock;
			xbee_packet_stream &pstream;
			// Map from file descriptor to client_info.
			std::tr1::unordered_map<int, client_info> fd_map;
			// Map from XBee address to which file descriptor cares about it.
			std::tr1::unordered_map<uint64_t, int> address_route;
			// Map from global frame ID to which file descriptor cares about it.
			int frameid_route[256];
			// Map from global frame ID to local frame ID.
			uint8_t frameid_revmap[256];

			void on_receive(std::vector<uint8_t> pkt);
	};

	daemon::daemon(file_descriptor listen_sock, xbee_packet_stream &pstream) : listen_sock(listen_sock), pstream(pstream) {
		std::fill(frameid_route, frameid_route + 256, -1);
		std::fill(frameid_revmap, frameid_revmap + 256, 0);
	}

	void daemon::run() {
		struct epoll_event events[8];
		uint8_t next_gfn = 1;

		// Attach receive callback for serial port.
		pstream.signal_received().connect(sigc::mem_fun(*this, &daemon::on_receive));

		// Create epoll fd.
		const file_descriptor &epollfd = create_epollfd();

		// Add listen sock to epoll fd.
		events[0].events = EPOLLIN;
		events[0].data.fd = listen_sock;
		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &events[0]) < 0)
			throw std::runtime_error("Cannot register listensock with epoll!");

		// Add serial port to epoll fd.
		events[0].events = EPOLLIN;
		events[0].data.fd = pstream.fd();
		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pstream.fd(), &events[0]) < 0)
			throw std::runtime_error("Cannot register serial port with epoll!");

		// Go into a loop.
		do {
			// Wait for something to happen.
			int nready = epoll_wait(epollfd, events, sizeof(events) / sizeof(*events), -1);
			if (nready < 0)
				throw std::runtime_error("Cannot wait for epoll event!");
			for (int i = 0; i < nready; i++) {
				if (events[i].data.fd == listen_sock) {
					// Listen socket is ready to do something.
					if (events[i].events & (EPOLLERR | EPOLLHUP)) {
						throw std::runtime_error("Error on listen socket!");
					} else if (events[i].events & EPOLLIN) {
						int newfd = accept(listen_sock, 0, 0);
						if (newfd >= 0) {
							events[i].events = EPOLLIN;
							events[i].data.fd = newfd;
							if (epoll_ctl(epollfd, EPOLL_CTL_ADD, newfd, &events[i]) < 0)
								throw std::runtime_error("Cannot register new socket with epoll!");
							fd_map[newfd] = client_info();
							send(newfd, "XBEE", 4, MSG_EOR | MSG_NOSIGNAL);
						}
					}
				} else if (events[i].data.fd == pstream.fd()) {
					// Serial port is ready to do something.
					if (events[i].events & (EPOLLERR | EPOLLHUP)) {
						throw std::runtime_error("Error on serial port!");
					} else {
						pstream.readable();
					}
				} else {
					// Client socket is ready to do something.
					if (events[i].events & (EPOLLERR | EPOLLHUP)) {
						// Socket error or closed.
						// Unregister from routing tables.
						for (std::tr1::unordered_set<uint8_t>::const_iterator j = fd_map[events[i].data.fd].frameid_reverse.begin(), jend = fd_map[events[i].data.fd].frameid_reverse.end(); j != jend; ++j) {
							frameid_route[*j] = -1;
							frameid_revmap[*j] = 0;
						}
						for (std::tr1::unordered_set<uint64_t>::const_iterator j = fd_map[events[i].data.fd].address_reverse.begin(), jend = fd_map[events[i].data.fd].address_reverse.end(); j != jend; ++j)
							address_route.erase(*j);
						// Remove from epoll.
						if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]) < 0)
							throw std::runtime_error("Cannot remove descriptor from epoll!");
						// Close descriptor.
						close(events[i].data.fd);
						// Remove from FD map.
						fd_map.erase(events[i].data.fd);
					} else if (events[i].events & EPOLLIN) {
						uint8_t buffer[65536];
						ssize_t ret = recv(events[i].data.fd, buffer, sizeof(buffer), MSG_DONTWAIT);
						if (ret < 0) {
							// Error or wouldblock.
							// If error, let epoll return EPOLLERR next time.
							// If wouldblock, just ignore it and keep polling.
						} else if (ret == 0) {
							// Remote peer closed.
							// Let epoll return EPOLLHUP next time.
						} else {
							// Data arrived on socket.
							// Check whether routing tables need updating.
							uint64_t destaddr;
							uint8_t local_frameid, global_frameid;
							switch (buffer[0]) {
								case 0x00:
								case 0x17:
									// Frame ID and destination address.
									// Extract destination address.
									destaddr = xbeeutil::address_from_bytes(&buffer[2]);
									// Remove any existing routing.
									if (address_route.count(destaddr)) {
										fd_map[address_route[destaddr]].address_reverse.erase(destaddr);
									}
									// Set new route.
									fd_map[events[i].data.fd].address_reverse.insert(destaddr);
									address_route[destaddr] = events[i].data.fd;
									// Fall through.

								case 0x08:
								case 0x09:
									// Frame ID.
									// Extract frame ID.
									local_frameid = buffer[1];
									if (local_frameid != 0) {
										// Allocate new global frame ID.
										global_frameid = next_gfn;
										next_gfn = (next_gfn == 255) ? (1) : (next_gfn + 1);
										// Swap global frame ID into packet.
										buffer[1] = global_frameid;
										// Remove any existing route.
										if (frameid_route[global_frameid] != -1) {
											fd_map[frameid_route[global_frameid]].frameid_reverse.erase(global_frameid);
										}
										// Set new route.
										fd_map[events[i].data.fd].frameid_reverse.insert(global_frameid);
										frameid_route[global_frameid] = events[i].data.fd;
										frameid_revmap[global_frameid] = local_frameid;
									}
									break;

								case 0x01:
									// 16-bit-address-frame (do not want).
									DPRINT("TX16 frame dropped (only 64-bit addresses allowed)");
									ret = -1;
									break;
							}

							// Push it to the serial port.
							pstream.send(buffer, ret);
						}
					}
				}
			}
		} while (!fd_map.empty());
	}

	void daemon::on_receive(std::vector<uint8_t> pkt) {
		int recipient = -1;

		// Check what type of packet it is.
		uint64_t srcaddr;
		uint8_t frameid;
		switch (pkt[0]) {
			case 0x80:
				// Route by sender address.
				srcaddr = xbeeutil::address_from_bytes(&pkt[1]);
				if (address_route.count(srcaddr))
					recipient = address_route[srcaddr];
				break;

			case 0x88:
			case 0x89:
			case 0x97:
				// Route by and translate frame ID.
				frameid = pkt[1];
				recipient = frameid_route[frameid];
				pkt[1] = frameid_revmap[frameid];
				break;
		}

		// Deliver the packet.
		if (recipient == -1) {
			// Broadcast delivery.
			for (std::tr1::unordered_map<int, client_info>::const_iterator i = fd_map.begin(), iend = fd_map.end(); i != iend; ++i)
				send(i->first, &pkt[0], pkt.size(), MSG_EOR | MSG_NOSIGNAL);
		} else {
			// Unicast delivery.
			send(recipient, &pkt[0], pkt.size(), MSG_EOR | MSG_NOSIGNAL);
		}
	}

	__attribute__((noreturn)) void run_daemon(file_descriptor listen_sock, xbee_packet_stream &pstream);
	void run_daemon(file_descriptor listen_sock, xbee_packet_stream &pstream) {
		try {
			{
				daemon d(listen_sock, pstream);
				d.run();
			}
			_exit(0);
		} catch (const std::exception &exp) {
			DPRINT(exp.what());
		} catch (...) {
		}
		_exit(1);
	}
}

namespace xbeedaemon {
	bool launch(file_descriptor &client_sock) {
		// Ignore the child-death signal.
		struct sigaction sigact;
		sigact.sa_handler = SIG_IGN;
		sigemptyset(&sigact.sa_mask);
		sigact.sa_flags = 0;
		if (sigaction(SIGCHLD, &sigact, 0) < 0)
			throw std::runtime_error("Cannot ignore SIGCHLD!");

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
		file_descriptor listen_sock(PF_UNIX, SOCK_SEQPACKET, 0);
		sockaddrs sa;
		sa.un.sun_family = AF_UNIX;
		if (socket_path.size() > sizeof(sa.un.sun_path)) throw std::runtime_error("Path too long!");
		std::copy(socket_path.begin(), socket_path.end(), &sa.un.sun_path[0]);
		std::fill(&sa.un.sun_path[socket_path.size()], &sa.un.sun_path[sizeof(sa.un.sun_path) / sizeof(*sa.un.sun_path)], '\0');
		if (bind(listen_sock, &sa.sa, sizeof(sa)) < 0) throw std::runtime_error("Cannot bind UNIX-domain socket!");
		if (listen(listen_sock, SOMAXCONN) < 0) throw std::runtime_error("Cannot listen on UNIX-domain socket!");

		// Connect another socket to it.
		file_descriptor temp_client_sock(PF_UNIX, SOCK_SEQPACKET, 0);
		if (connect(temp_client_sock, &sa.sa, sizeof(sa)) < 0) throw std::runtime_error("Cannot connect to UNIX-domain socket!");

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
			// Receive the signature.
			char buffer[4];
			if (recv(temp_client_sock, buffer, sizeof(buffer), 0) != sizeof(buffer))
				return false;
			if (buffer[0] != 'X' || buffer[1] != 'B' || buffer[2] != 'E' || buffer[3] != 'E') {
				return false;
			}
			client_sock = temp_client_sock;
			return true;
		} else {
			// This is the child process.
			// We need to keep the lock file, listen socket, and serial port.
			// Close all the others.
			for (int i = FIRST_CLOSE_FD; i < 65536; i++)
				if (i != listen_sock && i != lock_file && i != pstream.fd())
					close(i);
			// Enter a new session and process group.
			setsid();
			// Display the proper name.
			static const char PROCESS_NAME[] = "xbeed";
			prctl(PR_SET_NAME, PROCESS_NAME, 0, 0, 0);
			size_t arglen = 0;
			for (int i = 0; i < args::argc; i++)
				arglen += std::strlen(args::argv[i]) + 1;
			std::fill(args::argv[0], args::argv[0] + arglen, '\0');
			std::copy(PROCESS_NAME, PROCESS_NAME + sizeof(PROCESS_NAME), args::argv[0]);
			// Run as the multiplexing daemon.
			run_daemon(listen_sock, pstream);
		}
	}
}

