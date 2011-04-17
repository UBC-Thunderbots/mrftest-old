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
	 * The number of timesteps per second to simulate at in slow mode.
	 */
	const unsigned int SLOW_TIMESTEPS_PER_SECOND = 2;

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

Simulator::Simulator::Simulator(SimulatorEngine::Ptr engine) : engine(engine), listen_socket(create_listen_socket()), team1(*this, &team2, false), team2(*this, &team1, true), speed_mode_(::Simulator::Proto::SpeedMode::NORMAL), tick_scheduled(false), playtype(AI::Common::PlayType::HALT), frame_count(0), spinner_index(0) {
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

::Simulator::Proto::SpeedMode Simulator::Simulator::speed_mode() const {
	return speed_mode_;
}

void Simulator::Simulator::speed_mode(::Simulator::Proto::SpeedMode mode) {
	if (speed_mode_ != mode) {
		speed_mode_ = mode;
		team1.send_speed_mode();
		team2.send_speed_mode();
	}
}

AI::Common::PlayType Simulator::Simulator::play_type() const {
	return playtype;
}

void Simulator::Simulator::set_play_type(AI::Common::PlayType pt) {
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

	// Check the peer's credentials.
	ucred creds;
	socklen_t creds_len = sizeof(creds);
	if (getsockopt(sock->fd(), SOL_SOCKET, SO_PEERCRED, &creds, &creds_len) < 0) {
		throw SystemError("getsockopt", errno);
	}
	if (creds.uid != getuid() && creds.uid != 0) {
		throw std::runtime_error("Error on simulator socket: User ID of simulator does not match user ID of AI process");
	}

	// Make the socket blocking (individual operations can then be made non-blocking with MSG_DONTWAIT).
	try {
		sock->set_blocking(true);
	} catch (const SystemError &exp) {
		std::cout << "Error on pending socket: " << exp.what() << '\n';
		return true;
	}

	// Find a free team slot to put the AI into.
	Team *team = 0;
	if (!team1.has_connection()) {
		team = &team1;
	} else if (!team2.has_connection()) {
		team = &team2;
	} else {
		std::cout << "No space for new AI\n";
	}
	if (team) {
		// Get the socket ready to receive regular AI data.
		team->add_connection(Connection::create(sock));

		// Print a message.
		std::cout << "AI connected\n";
	}

	// Continue accepting incoming connections.
	return true;
}

void Simulator::Simulator::queue_load_state(FileDescriptor::Ptr fd) {
	load_state_pending_fd = fd;
}

void Simulator::Simulator::save_state(FileDescriptor::Ptr fd) const {
	engine->get_ball()->save_state(fd);
	team1.save_state(fd);
	team2.save_state(fd);
	engine->save_state(fd);
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

	switch (speed_mode_) {
		case ::Simulator::Proto::SpeedMode::NORMAL:
		case ::Simulator::Proto::SpeedMode::SLOW:
			// In normal-speed mode and slow mode, we first determine whether we've passed the deadline.
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
			break;

		case ::Simulator::Proto::SpeedMode::FAST:
			// In fast mode, we tick immediately.
			tick();
			break;
	}
}

void Simulator::Simulator::tick() {
	// Mark that no tick is scheduled.
	tick_scheduled = false;

	// If nobody is connected, do nothing.
	if (!team1.has_connection() && !team2.has_connection()) {
		return;
	}

	// If a state file has been queued for loading, load it now.
	if (load_state_pending_fd.is()) {
		engine->get_ball()->load_state(load_state_pending_fd);
		team1.load_state(load_state_pending_fd);
		team2.load_state(load_state_pending_fd);
		engine->load_state(load_state_pending_fd);
		load_state_pending_fd.reset();
	}

	// Tick the engine.
	engine->tick();

	// Update clients with the new world state.
	team1.send_tick(next_tick_game_monotonic_time);
	team2.send_tick(next_tick_game_monotonic_time);

	// Update the game monotonic time by exactly the size of a timestep.
	const timespec step_normal = { tv_sec: 1 / TIMESTEPS_PER_SECOND, tv_nsec: 1000000000L / TIMESTEPS_PER_SECOND - 1 / TIMESTEPS_PER_SECOND * 1000000000L, };
	const timespec step_slow = { tv_sec: 1 / SLOW_TIMESTEPS_PER_SECOND, tv_nsec: 1000000000L / SLOW_TIMESTEPS_PER_SECOND - 1 / SLOW_TIMESTEPS_PER_SECOND * 1000000000L, };
	const timespec step = speed_mode_ == ::Simulator::Proto::SpeedMode::SLOW ? step_slow : step_normal;
	timespec_add(next_tick_game_monotonic_time, step_normal, next_tick_game_monotonic_time);

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

