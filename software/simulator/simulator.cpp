#include "simulator/simulator.h"
#include "simulator/sockproto/proto.h"
#include "util/chdir.h"
#include "util/exception.h"
#include "util/misc.h"
#include "util/sockaddrs.h"
#include "util/timestep.h"
#include <cerrno>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

namespace {
	/**
	 * Creates the socket that listens for incoming connections from AI clients.
	 *
	 * \return the listening socket.
	 */
	FileDescriptor::Ptr create_listen_socket() {
		// Change to the cache directory where we will create the socket.
		const std::string &cache_dir = Glib::get_user_cache_dir();
		ScopedCHDir dir_changer(cache_dir.c_str());

		// Remove any existing socket.
		if (unlink(SIMULATOR_SOCKET_FILENAME) < 0 && errno != ENOENT) {
			throw SystemError("unlink(" SIMULATOR_SOCKET_FILENAME ")", errno);
		}

		// Create a new socket.
		FileDescriptor::Ptr sock = FileDescriptor::create_socket(AF_UNIX, SOCK_SEQPACKET, 0);
		sock->set_blocking(false);

		// Bind the new socket to the appropriate filename.
		SockAddrs sa;
		sa.un.sun_family = AF_UNIX;
		std::strcpy(sa.un.sun_path, SIMULATOR_SOCKET_FILENAME);
		if (bind(sock->fd(), &sa.sa, sizeof(sa.un)) < 0) {
			throw SystemError("bind(" SIMULATOR_SOCKET_FILENAME ")", errno);
		}

		// Listen for incoming connections.
		if (listen(sock->fd(), 8) < 0) {
			throw SystemError("listen", errno);
		}
		return sock;
	}
}

Simulator::Simulator::Simulator(SimulatorEngine::Ptr engine) : engine(engine), listen_socket(create_listen_socket()), team1(*this, &team2, false), team2(*this, &team1, true), fast(false), tick_scheduled(false), playtype(AI::Common::PlayType::HALT), frame_count(0), spinner_index(0) {
	next_tick_game_monotonic_time.tv_sec = 0;
	next_tick_game_monotonic_time.tv_nsec = 0;
	next_tick_phys_monotonic_time.tv_sec = 0;
	next_tick_phys_monotonic_time.tv_nsec = 0;
	last_fps_report_time.tv_sec = 0;
	last_fps_report_time.tv_nsec = 0;
	team1.signal_ready().connect(sigc::mem_fun(this, &Simulator::Simulator::check_tick));
	team2.signal_ready().connect(sigc::mem_fun(this, &Simulator::Simulator::check_tick));
	Glib::signal_io().connect(sigc::mem_fun(this, &Simulator::Simulator::on_ai_connecting), listen_socket->fd(), Glib::IO_IN);
	std::cout << "Simulator up and running\n";
}

bool Simulator::Simulator::is_fast() const {
	return fast;
}

void Simulator::Simulator::set_speed_mode(bool f) {
	if (fast != f) {
		fast = f;
		team1.send_speed_mode();
		team2.send_speed_mode();
	}
}

AI::Common::PlayType::PlayType Simulator::Simulator::play_type() const {
	return playtype;
}

void Simulator::Simulator::set_play_type(AI::Common::PlayType::PlayType pt) {
	if (pt != playtype) {
		playtype = pt;
		team1.send_play_type();
		team2.send_play_type();
	}
}

void Simulator::Simulator::encode_ball_state(Proto::S2ABallInfo &state, bool invert) {
	Point pos = engine->get_ball()->position();
	state.x = pos.x * (invert ? -1.0 : 1.0);
	state.y = pos.y * (invert ? -1.0 : 1.0);
}

bool Simulator::Simulator::on_ai_connecting(Glib::IOCondition) {
	// Accept an incoming connection.
	int raw_fd = accept(listen_socket->fd(), 0, 0);
	if (raw_fd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return true;
		} else {
			throw SystemError("accept", errno);
		}
	}
	FileDescriptor::Ptr sock = FileDescriptor::create_from_fd(raw_fd);
	raw_fd = -1;

	// Allow credentials to be received over this socket.
	const int yes = 1;
	if (setsockopt(sock->fd(), SOL_SOCKET, SO_PASSCRED, &yes, sizeof(yes)) < 0) {
		throw SystemError("setsockopt", errno);
	}

	// Make the socket blocking (individual operations can then be made non-blocking with MSG_DONTWAIT).
	try {
		sock->set_blocking(true);
	} catch (const SystemError &exp) {
		std::cout << "Error on pending socket: " << exp.what() << '\n';
		return true;
	}

	// Signal the first packet to be handled by on_pending_ai_readable.
	Glib::signal_io().connect(sigc::bind(sigc::mem_fun(this, &Simulator::Simulator::on_pending_ai_readable), sock), sock->fd(), Glib::IO_IN);

	// Continue accepting incoming connections.
	return true;
}

bool Simulator::Simulator::on_pending_ai_readable(Glib::IOCondition, FileDescriptor::Ptr sock) {
	// Receive the magic message from the AI, with attached credentials.
	char databuf[std::strlen(SIMULATOR_SOCKET_MAGIC1)], cmsgbuf[cmsg_space(sizeof(ucred))];
	iovec iov = { iov_base: databuf, iov_len: sizeof(databuf), };
	msghdr mh = { msg_name: 0, msg_namelen: 0, msg_iov: &iov, msg_iovlen: 1, msg_control: cmsgbuf, msg_controllen: sizeof(cmsgbuf), msg_flags: 0, };
	ssize_t bytes_read = recvmsg(sock->fd(), &mh, MSG_DONTWAIT);
	if (bytes_read < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return true;
		} else {
			try {
				throw SystemError("recvmsg", errno);
			} catch (const SystemError &exp) {
				std::cout << "Error on pending socket: " << exp.what() << '\n';
			}
			return false;
		}
	}
	if (!bytes_read) {
		return false;
	}
	if (bytes_read != std::strlen(SIMULATOR_SOCKET_MAGIC1) || (mh.msg_flags & MSG_TRUNC) || std::memcmp(databuf, SIMULATOR_SOCKET_MAGIC1, std::strlen(SIMULATOR_SOCKET_MAGIC1)) != 0) {
		std::cout << "Signature error on pending socket; are you running the same version of the simulator and the AI?\n";
		return false;
	}
	cmsghdr *ch = cmsg_firsthdr(&mh);
	if (!ch || ch->cmsg_level != SOL_SOCKET || ch->cmsg_type != SCM_CREDENTIALS) {
		std::cout << "Error on pending socket: No credentials transmitted\n";
		return false;
	}
	ucred creds;
	std::memcpy(&creds, cmsg_data(ch), sizeof(creds));
	if (creds.uid != getuid()) {
		std::cout << "Error on pending socket: User ID of AI process does not match user ID of simulator\n";
		return false;
	}

	// Find a free team slot to put the AI into.
	Team *team;
	if (!team1.has_connection()) {
		team = &team1;
	} else if (!team2.has_connection()) {
		team = &team2;
	} else {
		std::cout << "No space for new AI\n";
		return false;
	}

	// Send back the response magic packet with our own credentials attached.
	iov.iov_base = const_cast<char *>(SIMULATOR_SOCKET_MAGIC2);
	iov.iov_len = std::strlen(SIMULATOR_SOCKET_MAGIC2);
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;
	mh.msg_control = cmsgbuf;
	mh.msg_controllen = sizeof(cmsgbuf);
	ch = cmsg_firsthdr(&mh);
	ch->cmsg_level = SOL_SOCKET;
	ch->cmsg_type = SCM_CREDENTIALS;
	ch->cmsg_len = cmsg_len(sizeof(ucred));
	creds.pid = getpid();
	creds.uid = getuid();
	creds.gid = getgid();
	std::memcpy(cmsg_data(ch), &creds, sizeof(creds));
	mh.msg_flags = 0;
	if (sendmsg(sock->fd(), &mh, MSG_EOR | MSG_NOSIGNAL) < 0) {
		try {
			throw SystemError("sendmsg", errno);
		} catch (const SystemError &exp) {
			std::cout << "Error on AI socket: " << exp.what() << '\n';
		}
		return false;
	}

	// Get the socket ready to receive regular AI data.
	team->add_connection(Connection::create(sock));

	// Print a message.
	std::cout << "AI connected\n";

	// Further packets should not come here; they will go to on_ai_readable instead.
	return false;
}

void Simulator::Simulator::check_tick() {
	// If some team isn't ready yet, we can't tick.
	if (!(team1.ready() && team2.ready())) {
		return;
	}

	// If a tick has already been scheduled, don't schedule another one.
	if (tick_scheduled) {
		return;
	}

	if (fast) {
		// In fast mode, we tick immediately.
		tick();
	} else {
		// In normal-speed mode, we first determine whether we've passed the deadline.
		timespec now;
		timespec_now(now);
		if (timespec_cmp(now, next_tick_phys_monotonic_time) >= 0) {
			// It's time to tick.
			tick();
		} else {
			// Wait a bit before ticking.
			timespec diff;
			timespec_sub(next_tick_phys_monotonic_time, now, diff);
			Glib::signal_timeout().connect_once(sigc::mem_fun(this, &Simulator::Simulator::tick), timespec_to_millis(diff));
			tick_scheduled = true;
		}
	}
}

void Simulator::Simulator::tick() {
	// Mark that no tick is scheduled.
	tick_scheduled = false;

	// If nobody is connected, do nothing.
	if (!team1.has_connection() && !team2.has_connection()) {
		return;
	}

	// Tick the engine.
	engine->tick();

	// Update clients with the new world state.
	team1.send_tick(next_tick_game_monotonic_time);
	team2.send_tick(next_tick_game_monotonic_time);

	// Update the game monotonic time by exactly the size of a timestep.
	const timespec step = { tv_sec: 1 / TIMESTEPS_PER_SECOND, tv_nsec: 1000000000L / TIMESTEPS_PER_SECOND - 1 / TIMESTEPS_PER_SECOND * 1000000000L, };
	timespec_add(next_tick_game_monotonic_time, step, next_tick_game_monotonic_time);

	// Update the physical monotonic tick deadline to be as close as possible to one timestep forward from the previous tick.
	// However, clamp it to lie between the curren time and one timestep in the future.
	// The min-clamp prevents overloads from accumulating tick backlogs.
	// The max-clamp prevents fast-mode from pushing the deadline way into the future.
	timespec now;
	timespec_now(now);
	timespec_add(next_tick_phys_monotonic_time, step, next_tick_phys_monotonic_time);
	if (timespec_cmp(next_tick_phys_monotonic_time, now) < 0) {
		next_tick_phys_monotonic_time = now;
	}
	timespec now_plus_step;
	timespec_add(now, step, now_plus_step);
	if (timespec_cmp(next_tick_phys_monotonic_time, now_plus_step) > 0) {
		next_tick_phys_monotonic_time = now_plus_step;
	}

	// Display status line if half a second has passed since last report.
	++frame_count;
	timespec diff;
	timespec_sub(now, last_fps_report_time, diff);
	if (timespec_to_millis(diff) >= 500) {
		static const char SPINNER_CHARACTERS[] = "-\\|/";
		unsigned fps = (frame_count * 1000 + 500) / timespec_to_millis(diff);
		frame_count = 0;
		last_fps_report_time = now;
		std::cout << SPINNER_CHARACTERS[spinner_index++] << " [" << (team1.has_connection() ? '+' : ' ') << (team2.has_connection() ? '+' : ' ') << "] " << fps << "fps   \r";
		std::cout.flush();
		if (!SPINNER_CHARACTERS[spinner_index]) {
			spinner_index = 0;
		}
	}
}

