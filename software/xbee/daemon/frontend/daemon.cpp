#include "util/fd.h"
#include "util/sockaddrs.h"
#include "xbee/daemon/frontend/already_running.h"
#include "xbee/daemon/frontend/client.h"
#include "xbee/daemon/frontend/daemon.h"
#include "xbee/shared/packettypes.h"
#include <algorithm>
#include <stdexcept>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	FileDescriptor open_and_lock_lock_file() {
		// Calculate filenames.
		const Glib::ustring &cache_dir = Glib::get_user_cache_dir();
		const std::string &lock_path = Glib::filename_from_utf8(cache_dir + "/thunderbots-xbeed-lock");

		// Try to lock the lock file.
		FileDescriptor lock_file(lock_path.c_str(), O_CREAT | O_RDWR);
		if (flock(lock_file, LOCK_EX | LOCK_NB) < 0) {
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				throw AlreadyRunning();
			} else {
				throw std::runtime_error("Cannot lock lock file!");
			}
		}

		return lock_file;
	}

	FileDescriptor create_listening_socket() {
		// Compute the path to the socket.
		const Glib::ustring &cache_dir = Glib::get_user_cache_dir();
		const std::string &socket_path = Glib::filename_from_utf8(cache_dir + "/thunderbots-xbeed-sock");

		// Delete any existing socket.
		unlink(socket_path.c_str());

		// Create the listening socket.
		FileDescriptor listen_sock(PF_UNIX, SOCK_SEQPACKET, 0);

		// Bind it to the path.
		SockAddrs sa;
		sa.un.sun_family = AF_UNIX;
		if (socket_path.size() > sizeof(sa.un.sun_path)) {
			throw std::runtime_error("Path too long!");
		}
		std::copy(socket_path.begin(), socket_path.end(), &sa.un.sun_path[0]);
		std::fill(&sa.un.sun_path[socket_path.size()], &sa.un.sun_path[sizeof(sa.un.sun_path) / sizeof(*sa.un.sun_path)], '\0');
		if (bind(listen_sock, &sa.sa, sizeof(sa.un)) < 0) {
			throw std::runtime_error("Cannot bind UNIX-domain socket!");
		}

		// Listen for incoming connections.
		if (listen(listen_sock, SOMAXCONN) < 0) {
			throw std::runtime_error("Cannot listen on UNIX-domain socket!");
		}

		return listen_sock;
	}
}

XBeeDaemon::XBeeDaemon(BackEnd &backend) : backend(backend), frame_number_allocator(1, 255), id16_allocator(1, 0xFFFD), scheduler(*this), universe_claimed(false), lock_file(open_and_lock_lock_file()), listen_sock(create_listening_socket()), allocated_rundata_indices(XBeePacketTypes::MAX_DRIVE_ROBOTS, false) {
	// Blank the run data index reverse.
	std::fill(run_data_index_reverse, run_data_index_reverse + XBeePacketTypes::MAX_DRIVE_ROBOTS, UINT64_C(0));

	// Initialize the rwlock.
	pthread_rwlockattr_t attrib;
	if (pthread_rwlockattr_init(&attrib) != 0) {
		throw std::runtime_error("Cannot initialize rwlock attributes!");
	}
	if (pthread_rwlockattr_setpshared(&attrib, PTHREAD_PROCESS_SHARED) != 0) {
		throw std::runtime_error("Cannot set process-shared flag in rwlock attributes!");
	}
	if (pthread_rwlock_init(&shm->lock, &attrib) != 0) {
		throw std::runtime_error("Cannot create rwlock in shared memory block!");
	}
	if (pthread_rwlockattr_destroy(&attrib) != 0) {
		throw std::runtime_error("Cannot destroy rwlock attributes!");
	}

	// Compose and send a request to set the local XBee's 16-bit address to 0.
	XBeePacketTypes::AT_REQUEST<2> packet;
	packet.apiid = XBeePacketTypes::AT_REQUEST_APIID;
	packet.frame = frame_number_allocator.alloc();
	packet.command[0] = 'M';
	packet.command[1] = 'Y';
	packet.value[0] = 0x00;
	packet.value[1] = 0x00;
	scheduler.queue(XBeeRequest::create(&packet, sizeof(packet), true));

	// Connect a signal to fire when clients connect.
	Glib::signal_io().connect(sigc::mem_fun(this, &XBeeDaemon::on_accept), listen_sock, Glib::IO_IN);
}

XBeeDaemon::~XBeeDaemon() {
	pthread_rwlock_destroy(&shm->lock);
}

uint8_t XBeeDaemon::alloc_rundata_index() {
	for (unsigned int i = 0; i < allocated_rundata_indices.size(); ++i) {
		if (!allocated_rundata_indices[i]) {
			allocated_rundata_indices[i] = true;
			return i;
		}
	}
	return UINT8_C(0xFF);
}

void XBeeDaemon::free_rundata_index(uint8_t index) {
	assert(index < allocated_rundata_indices.size());
	assert(allocated_rundata_indices[index]);
	allocated_rundata_indices[index] = false;
	run_data_index_reverse[index] = 0;
}

void XBeeDaemon::check_shutdown() {
	check_shutdown_firer.disconnect();
	if (XBeeClient::any_connected()) {
		return;
	}
	for (std::unordered_map<uint64_t, XBeeRobot::Ptr>::iterator i = robots.begin(), iend = robots.end(); i != iend; ++i) {
		const XBeeRobot::Ptr state(i->second);
		if (state->freeing()) {
			check_shutdown_firer = state->signal_resources_freed.connect(sigc::mem_fun(this, &XBeeDaemon::check_shutdown));
			return;
		}
	}
	signal_last_client_disconnected.emit();
}

bool XBeeDaemon::on_accept(Glib::IOCondition) {
	int newfd = accept(listen_sock, 0, 0);
	if (newfd >= 0) {
		FileDescriptor fd(newfd);
		if (!universe_claimed) {
			XBeeClient::create(fd, *this);
			check_shutdown_firer.disconnect();
		}
	}
	return true;
}

