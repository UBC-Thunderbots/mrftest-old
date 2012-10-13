#include "ai/backend/simulator/backend.h"
#include "ai/common/robot.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/misc.h"
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/window.h>
#include <sys/stat.h>
#include <sys/types.h>

AI::BE::Simulator::SecondaryUIControls::SecondaryUIControls(const std::string &load_filename) : speed_label("Speed:"), speed_normal(speed_group, "Normal"), speed_fast(speed_group, "Fast"), speed_slow(speed_group, "Slow"), players_label("Players:"), players_hbox(Gtk::BUTTONBOX_SPREAD), players_add("+"), players_remove("−"), state_file_name(load_filename), state_file_label("State:"), state_file_button("…"), state_file_hbox(Gtk::BUTTONBOX_SPREAD), state_file_load_button("Load"), state_file_save_button("Save") {
	speed_normal.set_sensitive(false);
	speed_fast.set_sensitive(false);
	speed_slow.set_sensitive(false);
	speed_hbox.pack_start(speed_normal, Gtk::PACK_EXPAND_WIDGET);
	speed_hbox.pack_start(speed_fast, Gtk::PACK_EXPAND_WIDGET);
	speed_hbox.pack_start(speed_slow, Gtk::PACK_EXPAND_WIDGET);

	players_add.set_sensitive(false);
	players_remove.set_sensitive(false);
	players_hbox.pack_start(players_add);
	players_hbox.pack_start(players_remove);

	state_file_entry.set_editable(false);
	state_file_entry.set_text(Glib::filename_to_utf8(state_file_name));
	state_file_button.signal_clicked().connect(sigc::mem_fun(this, &SecondaryUIControls::on_state_file_button_clicked));

	state_file_load_button.set_sensitive(false);
	state_file_save_button.set_sensitive(!state_file_name.empty());
	state_file_hbox.pack_start(state_file_load_button);
	state_file_hbox.pack_start(state_file_save_button);
}

void AI::BE::Simulator::SecondaryUIControls::on_state_file_button_clicked() {
	Gtk::Window *win = dynamic_cast<Gtk::Window *>(state_file_button.get_toplevel());
	Gtk::FileChooserDialog fc(*win, "Choose State File", Gtk::FILE_CHOOSER_ACTION_SAVE);
	if (!state_file_name.empty()) {
		fc.set_filename(state_file_name);
	}
	fc.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
	fc.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	fc.set_default_response(Gtk::RESPONSE_OK);
	if (fc.run() == Gtk::RESPONSE_OK) {
		state_file_name = fc.get_filename().raw();
		state_file_entry.set_text(Glib::filename_to_utf8(state_file_name));
		state_file_save_button.set_sensitive();
	}
}

unsigned int AI::BE::Simulator::SecondaryUIControls::rows() const {
	return 4;
}

void AI::BE::Simulator::SecondaryUIControls::attach(Gtk::Table &t, unsigned int row) {
	t.attach(speed_label, 0, 1, row + 0, row + 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(speed_hbox, 1, 3, row + 0, row + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(players_label, 0, 1, row + 1, row + 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(players_hbox, 1, 3, row + 1, row + 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(state_file_label, 0, 1, row + 2, row + 4, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(state_file_entry, 1, 2, row + 2, row + 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(state_file_button, 2, 3, row + 2, row + 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(state_file_hbox, 1, 3, row + 3, row + 4, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
}

FileDescriptor AI::BE::Simulator::connect_to_simulator() {
	// Change to the cache directory where the simulator creates its socket.
	const std::string &cache_dir = Glib::get_user_cache_dir();
	ScopedCHDir dir_changer(cache_dir.c_str());

	// Create a new socket.
	FileDescriptor sock = FileDescriptor::create_socket(AF_UNIX, SOCK_SEQPACKET, 0);
	sock.set_blocking(true);

	// Connect to the simulator's socket.
	sockaddr_un sa;
	sa.sun_family = AF_UNIX;
	std::strcpy(sa.sun_path, SIMULATOR_SOCKET_FILENAME);
	if (connect(sock.fd(), reinterpret_cast<const sockaddr *>(&sa), sizeof(sa)) < 0) {
		throw SystemError("connect", errno);
	}

	// Check the peer's credentials.
	ucred creds;
	socklen_t creds_len = sizeof(creds);
	if (getsockopt(sock.fd(), SOL_SOCKET, SO_PEERCRED, &creds, &creds_len) < 0) {
		throw SystemError("getsockopt", errno);
	}
	if (creds.uid != getuid() && creds.uid != 0) {
		throw std::runtime_error("Error on simulator socket: User ID of simulator does not match user ID of AI process");
	}

	// We're alive.
	return sock;
}

AI::BE::Simulator::Backend::Backend(const std::string &load_filename) : sock(connect_to_simulator()), friendly_(*this), enemy_(*this), secondary_controls(load_filename), dragging_ball(false), dragging_pattern(std::numeric_limits<unsigned int>::max()) {
	secondary_controls.speed_normal.signal_toggled().connect(sigc::mem_fun(this, &Backend::on_speed_toggled));
	secondary_controls.speed_fast.signal_toggled().connect(sigc::mem_fun(this, &Backend::on_speed_toggled));
	secondary_controls.speed_slow.signal_toggled().connect(sigc::mem_fun(this, &Backend::on_speed_toggled));
	secondary_controls.players_add.signal_clicked().connect(sigc::mem_fun(this, &Backend::on_players_add_clicked));
	secondary_controls.players_remove.signal_clicked().connect(sigc::mem_fun(this, &Backend::on_players_remove_clicked));
	secondary_controls.state_file_load_button.signal_clicked().connect(sigc::mem_fun(this, &Backend::on_state_file_load_clicked));
	secondary_controls.state_file_save_button.signal_clicked().connect(sigc::mem_fun(this, &Backend::on_state_file_save_clicked));
	Glib::signal_io().connect(sigc::mem_fun(this, &Backend::on_packet), sock.fd(), Glib::IO_IN);
	playtype_override().signal_changed().connect(sigc::mem_fun(this, &Backend::update_playtype));

	if (!load_filename.empty()) {
		on_state_file_load_clicked();
	}

	field_.update(6.05, 7.40, 4.05, 5.40, 0.70, 0.50, 0.80, 0.35);
}

void AI::BE::Simulator::Backend::send_packet(const ::Simulator::Proto::A2SPacket &packet, const FileDescriptor &ancillary_fd) {
	iovec iov;
	iov.iov_base = const_cast< ::Simulator::Proto::A2SPacket *>(&packet);
	iov.iov_len = sizeof(packet);

	msghdr msgh;
	msgh.msg_name = 0;
	msgh.msg_namelen = 0;
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_control = 0;
	msgh.msg_controllen = 0;
	msgh.msg_flags = 0;

	char cmsgbuf[cmsg_space(sizeof(int))];
	if (ancillary_fd.is()) {
		msgh.msg_control = cmsgbuf;
		msgh.msg_controllen = sizeof(cmsgbuf);
		cmsg_firsthdr(&msgh)->cmsg_len = cmsg_len(sizeof(int));
		cmsg_firsthdr(&msgh)->cmsg_level = SOL_SOCKET;
		cmsg_firsthdr(&msgh)->cmsg_type = SCM_RIGHTS;
		int fd = ancillary_fd.fd();
		std::memcpy(cmsg_data(cmsg_firsthdr(&msgh)), &fd, sizeof(fd));
	}
	if (sendmsg(sock.fd(), &msgh, MSG_NOSIGNAL) < 0) {
		throw SystemError("sendmsg", errno);
	}
}

const AI::BE::Team<AI::BE::Player> &AI::BE::Simulator::Backend::friendly_team() const {
	return friendly_;
}

const AI::BE::Team<AI::BE::Robot> &AI::BE::Simulator::Backend::enemy_team() const {
	return enemy_;
}

std::size_t AI::BE::Simulator::Backend::visualizable_num_robots() const {
	return friendly_.size() + enemy_.size();
}

Visualizable::Robot::Ptr AI::BE::Simulator::Backend::visualizable_robot(std::size_t i) const {
	if (i < friendly_.size()) {
		return friendly_.get(i);
	} else {
		return enemy_.get(i - friendly_.size());
	}
}

void AI::BE::Simulator::Backend::mouse_pressed(Point p, unsigned int btn) {
	if (btn == 1) {
		dragging_ball = false;
		dragging_pattern = std::numeric_limits<unsigned int>::max();
		ball_.should_highlight = false;
		for (std::size_t i = 0; i < friendly_.size(); ++i) {
			friendly_.get_impl(i)->stop_drag();
		}
		if ((p - ball_.position()).len() < Ball::RADIUS) {
			dragging_ball = true;
			dragging_pattern = std::numeric_limits<unsigned int>::max();
			ball_.should_highlight = true;
		} else {
			for (std::size_t i = 0; i < friendly_.size(); ++i) {
				AI::BE::Simulator::Player::Ptr bot = friendly_.get_impl(i);
				if ((p - bot->position(0.0)).len() < AI::Common::Robot::MAX_RADIUS) {
					bot->start_drag();
					dragging_pattern = bot->pattern();
					break;
				}
			}
		}
	}
}

void AI::BE::Simulator::Backend::mouse_released(Point, unsigned int btn) {
	if (btn == 1) {
		mouse_exited();
	}
}

void AI::BE::Simulator::Backend::mouse_exited() {
	dragging_ball = false;
	dragging_pattern = std::numeric_limits<unsigned int>::max();
	ball_.should_highlight = false;
	for (std::size_t i = 0; i < friendly_.size(); ++i) {
		friendly_.get_impl(i)->stop_drag();
	}
}

void AI::BE::Simulator::Backend::mouse_moved(Point p) {
	if (dragging_ball) {
		::Simulator::Proto::A2SPacket packet;
		std::memset(&packet, 0, sizeof(packet));
		packet.type = ::Simulator::Proto::A2SPacketType::DRAG_BALL;
		packet.drag.pattern = 0;
		packet.drag.x = p.x;
		packet.drag.y = p.y;
		send_packet(packet);
	} else if (dragging_pattern != std::numeric_limits<unsigned int>::max()) {
		bool found = false;
		for (std::size_t i = 0; i < friendly_.size() && !found; ++i) {
			if (friendly_.get_impl(i)->pattern() == dragging_pattern) {
				found = true;
			}
		}
		if (found) {
			::Simulator::Proto::A2SPacket packet;
			std::memset(&packet, 0, sizeof(packet));
			packet.type = ::Simulator::Proto::A2SPacketType::DRAG_PLAYER;
			packet.drag.pattern = dragging_pattern;
			packet.drag.x = p.x;
			packet.drag.y = p.y;
			send_packet(packet);
		} else {
			dragging_pattern = std::numeric_limits<unsigned int>::max();
		}
	}
}

unsigned int AI::BE::Simulator::Backend::main_ui_controls_table_rows() const {
	return 0;
}

void AI::BE::Simulator::Backend::main_ui_controls_attach(Gtk::Table &, unsigned int) {
}

unsigned int AI::BE::Simulator::Backend::secondary_ui_controls_table_rows() const {
	return secondary_controls.rows();
}

void AI::BE::Simulator::Backend::secondary_ui_controls_attach(Gtk::Table &t, unsigned int row) {
	secondary_controls.attach(t, row);
}

bool AI::BE::Simulator::Backend::on_packet(Glib::IOCondition) {
	// Receive a packet.
	::Simulator::Proto::S2APacket packet;
	iovec iov;
	iov.iov_base = &packet;
	iov.iov_len = sizeof(packet);

	msghdr mh;
	mh.msg_name = 0;
	mh.msg_namelen = 0;
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;
	mh.msg_control = 0;
	mh.msg_controllen = 0;
	mh.msg_flags = 0;

	ssize_t rc = recvmsg(sock.fd(), &mh, MSG_DONTWAIT);
	if (rc < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return true;
		}
		throw SystemError("recvmsg", errno);
	} else if (rc == 0) {
		throw std::runtime_error("Simulator unexpectedly closed socket");
	} else if (rc != sizeof(packet) || (mh.msg_flags & MSG_TRUNC)) {
		throw std::runtime_error("Simulator sent bad packet");
	}

	// Decide what to do based on the type code.
	switch (packet.type) {
		case ::Simulator::Proto::S2APacketType::TICK:
		{
			// Record the current in-world monotonic time.
			monotonic_time_ = packet.world_state.stamp;

			// Record the current physical monotonic time.
			timespec before;
			if (clock_gettime(CLOCK_MONOTONIC, &before) < 0) {
				throw SystemError(u8"clock_gettime(CLOCK_MONOTONIC)", errno);
			}

			// Update the objects with the newly-received data and lock in their predictors.
			ball_.add_field_data({packet.world_state.ball.x, packet.world_state.ball.y}, monotonic_time_);
			ball_.lock_time(monotonic_time_);
			friendly_.pre_tick(packet.world_state.friendly, packet.world_state.friendly_score, monotonic_time_);
			enemy_.pre_tick(packet.world_state.enemy, packet.world_state.enemy_score, monotonic_time_);

			// Run the AI.
			signal_tick().emit();

			// Push results back to the simulator.
			send_orders();

			// Compute time taken.
			timespec after;
			if (clock_gettime(CLOCK_MONOTONIC, &after) < 0) {
				throw SystemError(u8"clock_gettime(CLOCK_MONOTONIC)", errno);
			}
			unsigned int compute_time = timespec_to_nanos(timespec_sub(after, before));

			// Notify anyone interested in the finish of a tick.
			signal_post_tick().emit(compute_time);

			// Update sensitivities of player add/remove buttons.
			secondary_controls.players_add.set_sensitive(friendly_.size() < ::Simulator::Proto::MAX_PLAYERS_PER_TEAM);
			secondary_controls.players_remove.set_sensitive(friendly_.size() > 0);
			secondary_controls.state_file_load_button.set_sensitive(!secondary_controls.state_file_name.empty());
			return true;
		}

		case ::Simulator::Proto::S2APacketType::SPEED_MODE:
			// Update the UI controls.
			switch (packet.speed_mode) {
				case ::Simulator::Proto::SpeedMode::NORMAL:
					secondary_controls.speed_normal.set_active();
					break;

				case ::Simulator::Proto::SpeedMode::FAST:
					secondary_controls.speed_fast.set_active();
					break;

				case ::Simulator::Proto::SpeedMode::SLOW:
					secondary_controls.speed_slow.set_active();
					break;
			}

			// Make both radio buttons sensitive.
			secondary_controls.speed_normal.set_sensitive();
			secondary_controls.speed_fast.set_sensitive();
			secondary_controls.speed_slow.set_sensitive();
			return true;
	}

	throw std::runtime_error("Simulator sent bad packet type");
}

void AI::BE::Simulator::Backend::send_orders() {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2SPacketType::PLAYERS;
	friendly_.encode_orders(packet.players);
	send_packet(packet);
}

void AI::BE::Simulator::Backend::update_playtype() {
	if (playtype_override() != AI::Common::PlayType::NONE) {
		playtype_rw() = static_cast<AI::Common::PlayType>(playtype_override());
	} else {
		playtype_rw() = AI::Common::PlayType::HALT;
	}
}

void AI::BE::Simulator::Backend::on_speed_toggled() {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2SPacketType::SET_SPEED;
	if (secondary_controls.speed_fast.get_active()) {
		packet.speed_mode = ::Simulator::Proto::SpeedMode::FAST;
	} else if (secondary_controls.speed_slow.get_active()) {
		packet.speed_mode = ::Simulator::Proto::SpeedMode::SLOW;
	} else {
		packet.speed_mode = ::Simulator::Proto::SpeedMode::NORMAL;
	}
	send_packet(packet);
}

void AI::BE::Simulator::Backend::on_players_add_clicked() {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2SPacketType::ADD_PLAYER;
	for (packet.pattern = 0; pattern_exists(packet.pattern); ++packet.pattern) {
	}
	send_packet(packet);
	secondary_controls.players_add.set_sensitive(false);
	secondary_controls.players_remove.set_sensitive(false);
	secondary_controls.state_file_load_button.set_sensitive(false);
}

void AI::BE::Simulator::Backend::on_players_remove_clicked() {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2SPacketType::REMOVE_PLAYER;
	for (packet.pattern = 0; !pattern_exists(packet.pattern); ++packet.pattern) {
	}
	send_packet(packet);
	secondary_controls.players_add.set_sensitive(false);
	secondary_controls.players_remove.set_sensitive(false);
	secondary_controls.state_file_load_button.set_sensitive(false);
}

bool AI::BE::Simulator::Backend::pattern_exists(unsigned int pattern) {
	for (std::size_t i = 0; i < friendly_.size(); ++i) {
		if (friendly_.get(i)->pattern() == pattern) {
			return true;
		}
	}
	return false;
}

void AI::BE::Simulator::Backend::on_state_file_load_clicked() {
	FileDescriptor fd = FileDescriptor::create_open(secondary_controls.state_file_name.c_str(), O_RDONLY, 0);
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2SPacketType::LOAD_STATE;
	send_packet(packet, fd);
}

void AI::BE::Simulator::Backend::on_state_file_save_clicked() {
	FileDescriptor fd = FileDescriptor::create_open(secondary_controls.state_file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2SPacketType::SAVE_STATE;
	send_packet(packet, fd);
}

AI::BE::Simulator::BackendFactory::BackendFactory() : AI::BE::BackendFactory("Simulator") {
}

void AI::BE::Simulator::BackendFactory::create_backend(const std::string &load_filename, unsigned int, int, std::function<void(AI::BE::Backend &)> cb) const {
	Backend be(load_filename);
	cb(be);
}

namespace {
	/**
	 * The global instance of BackendFactory.
	 */
	AI::BE::Simulator::BackendFactory factory_instance;
}

AI::BE::BackendFactory &AI::BE::Simulator::Backend::factory() const {
	return factory_instance;
}

