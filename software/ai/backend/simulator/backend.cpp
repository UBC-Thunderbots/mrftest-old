#include "ai/backend/simulator/backend.h"
#include "util/fd.h"
#include "util/misc.h"
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

AI::BE::Simulator::MainUIControls::MainUIControls() : playtype_label("Play type (sim):") {
	for (unsigned int i = 0; i < static_cast<unsigned int>(AI::Common::PlayType::NONE); ++i) {
		playtype_combo.append_text(AI::Common::PlayTypeInfo::to_string(AI::Common::PlayTypeInfo::of_int(i)));
	}
	playtype_combo.set_active_text(AI::Common::PlayTypeInfo::to_string(AI::Common::PlayType::HALT));
	playtype_combo.set_sensitive(false);
}

unsigned int AI::BE::Simulator::MainUIControls::rows() const {
	return 1;
}

void AI::BE::Simulator::MainUIControls::attach(Gtk::Table &t, unsigned int row) {
	t.attach(playtype_label, 0, 1, row, row + 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(playtype_combo, 1, 3, row, row + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
}

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

FileDescriptor::Ptr AI::BE::Simulator::connect_to_simulator() {
	// Change to the cache directory where the simulator creates its socket.
	const std::string &cache_dir = Glib::get_user_cache_dir();
	ScopedCHDir dir_changer(cache_dir.c_str());

	// Create a new socket.
	FileDescriptor::Ptr sock = FileDescriptor::create_socket(AF_UNIX, SOCK_SEQPACKET, 0);
	sock->set_blocking(true);

	// Connect to the simulator's socket.
	SockAddrs sa;
	sa.un.sun_family = AF_UNIX;
	std::strcpy(sa.un.sun_path, SIMULATOR_SOCKET_FILENAME);
	if (connect(sock->fd(), &sa.sa, sizeof(sa.un)) < 0) {
		throw SystemError("connect", errno);
	}

	// Check the peer's credentials.
	ucred creds;
	socklen_t creds_len = sizeof(creds);
	if (getsockopt(sock->fd(), SOL_SOCKET, SO_PEERCRED, &creds, &creds_len) < 0) {
		throw SystemError("getsockopt", errno);
	}
	if (creds.uid != getuid() && creds.uid != 0) {
		throw std::runtime_error("Error on simulator socket: User ID of simulator does not match user ID of AI process");
	}

	// We're alive.
	return sock;
}

AI::BE::Simulator::Backend::Backend(const std::string &load_filename) : sock(connect_to_simulator()), ball_(*this), friendly_(*this), simulator_playtype(AI::Common::PlayType::HALT), secondary_controls(load_filename) {
	monotonic_time_.tv_sec = 0;
	monotonic_time_.tv_nsec = 0;
	main_controls.playtype_combo.signal_changed().connect(sigc::mem_fun(this, &Backend::on_sim_playtype_changed));
	secondary_controls.speed_normal.signal_toggled().connect(sigc::mem_fun(this, &Backend::on_speed_toggled));
	secondary_controls.speed_fast.signal_toggled().connect(sigc::mem_fun(this, &Backend::on_speed_toggled));
	secondary_controls.speed_slow.signal_toggled().connect(sigc::mem_fun(this, &Backend::on_speed_toggled));
	secondary_controls.players_add.signal_clicked().connect(sigc::mem_fun(this, &Backend::on_players_add_clicked));
	secondary_controls.players_remove.signal_clicked().connect(sigc::mem_fun(this, &Backend::on_players_remove_clicked));
	secondary_controls.state_file_load_button.signal_clicked().connect(sigc::mem_fun(this, &Backend::on_state_file_load_clicked));
	secondary_controls.state_file_save_button.signal_clicked().connect(sigc::mem_fun(this, &Backend::on_state_file_save_clicked));
	Glib::signal_io().connect(sigc::mem_fun(this, &Backend::on_packet), sock->fd(), Glib::IO_IN);
	friendly_.score_prop.signal_changed().connect(signal_score_changed().make_slot());
	enemy_.score_prop.signal_changed().connect(signal_score_changed().make_slot());
	playtype_override().signal_changed().connect(sigc::mem_fun(this, &Backend::update_playtype));

	if (!load_filename.empty()) {
		on_state_file_load_clicked();
	}
}

void AI::BE::Simulator::Backend::send_packet(const ::Simulator::Proto::A2SPacket &packet, FileDescriptor::Ptr ancillary_fd) {
	iovec iov = { iov_base: const_cast< ::Simulator::Proto::A2SPacket *>(&packet), iov_len: sizeof(packet), };
	msghdr msgh = { msg_name: 0, msg_namelen: 0, msg_iov: &iov, msg_iovlen: 1, msg_control: 0, msg_controllen: 0, msg_flags: 0, };
	char cmsgbuf[cmsg_space(sizeof(int))];
	if (ancillary_fd.is()) {
		msgh.msg_control = cmsgbuf;
		msgh.msg_controllen = sizeof(cmsgbuf);
		cmsg_firsthdr(&msgh)->cmsg_len = cmsg_len(sizeof(int));
		cmsg_firsthdr(&msgh)->cmsg_level = SOL_SOCKET;
		cmsg_firsthdr(&msgh)->cmsg_type = SCM_RIGHTS;
		int fd = ancillary_fd->fd();
		std::memcpy(cmsg_data(cmsg_firsthdr(&msgh)), &fd, sizeof(fd));
	}
	if (sendmsg(sock->fd(), &msgh, MSG_NOSIGNAL) < 0) {
		throw SystemError("sendmsg", errno);
	}
}

const AI::BE::Simulator::Field &AI::BE::Simulator::Backend::field() const {
	return field_;
}

const AI::BE::Simulator::Ball &AI::BE::Simulator::Backend::ball() const {
	return ball_;
}

AI::BE::Simulator::FriendlyTeam &AI::BE::Simulator::Backend::friendly_team() {
	return friendly_;
}

const AI::BE::Simulator::FriendlyTeam &AI::BE::Simulator::Backend::friendly_team() const {
	return friendly_;
}

const AI::BE::Simulator::EnemyTeam &AI::BE::Simulator::Backend::enemy_team() const {
	return enemy_;
}

timespec AI::BE::Simulator::Backend::monotonic_time() const {
	return monotonic_time_;
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
	signal_mouse_pressed.emit(p, btn);
}

void AI::BE::Simulator::Backend::mouse_released(Point p, unsigned int btn) {
	signal_mouse_released.emit(p, btn);
}

void AI::BE::Simulator::Backend::mouse_exited() {
	signal_mouse_exited.emit();
}

void AI::BE::Simulator::Backend::mouse_moved(Point p) {
	signal_mouse_moved.emit(p);
}

unsigned int AI::BE::Simulator::Backend::main_ui_controls_table_rows() const {
	return main_controls.rows();
}

void AI::BE::Simulator::Backend::main_ui_controls_attach(Gtk::Table &t, unsigned int row) {
	main_controls.attach(t, row);
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
	iovec iov = { iov_base: &packet, iov_len: sizeof(packet), };
	msghdr mh = { msg_name: 0, msg_namelen: 0, msg_iov: &iov, msg_iovlen: 1, msg_control: 0, msg_controllen: 0, msg_flags: 0 };
	ssize_t rc = recvmsg(sock->fd(), &mh, MSG_DONTWAIT);
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
			// Record the current monotonic time.
			monotonic_time_ = packet.world_state.stamp;

			// Update the objects with the newly-received data and lock in their predictors.
			ball_.pre_tick(packet.world_state.ball, monotonic_time_);
			friendly_.pre_tick(packet.world_state.friendly, packet.world_state.friendly_score, monotonic_time_);
			enemy_.pre_tick(packet.world_state.enemy, packet.world_state.enemy_score, monotonic_time_);

			// Run the AI.
			signal_tick().emit();

			// Push results back to the simulator.
			send_orders();

			// Notify anyone interested in the finish of a tick.
			signal_post_tick().emit();

			// Update sensitivities of player add/remove buttons.
			secondary_controls.players_add.set_sensitive(friendly_.size() < ::Simulator::Proto::MAX_PLAYERS_PER_TEAM);
			secondary_controls.players_remove.set_sensitive(friendly_.size() > 0);
			secondary_controls.state_file_load_button.set_sensitive(!secondary_controls.state_file_name.empty());
			return true;

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

		case ::Simulator::Proto::S2APacketType::PLAY_TYPE:
			if (packet.playtype != AI::Common::PlayType::NONE) {
				// Record the master play type.
				simulator_playtype = packet.playtype;

				// Update and make sensitive the master play type combo box.
				main_controls.playtype_combo.set_active_text(AI::Common::PlayTypeInfo::to_string(simulator_playtype));
				main_controls.playtype_combo.set_sensitive();

				// Update the current play type, which is a function of the master and override types.
				update_playtype();
				return true;
			} else {
				throw std::runtime_error("Simulator sent illegal play type");
			}
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
		playtype_rw() = playtype_override();
	} else {
		playtype_rw() = simulator_playtype;
	}
}

void AI::BE::Simulator::Backend::on_sim_playtype_changed() {
	if (main_controls.playtype_combo.get_active_row_number() != -1) {
		::Simulator::Proto::A2SPacket packet;
		std::memset(&packet, 0, sizeof(packet));
		packet.type = ::Simulator::Proto::A2SPacketType::PLAY_TYPE;
		packet.playtype = AI::Common::PlayTypeInfo::of_int(main_controls.playtype_combo.get_active_row_number());
		send_packet(packet);
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
	FileDescriptor::Ptr fd = FileDescriptor::create_open(secondary_controls.state_file_name.c_str(), O_RDONLY, 0);
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2SPacketType::LOAD_STATE;
	send_packet(packet, fd);
}

void AI::BE::Simulator::Backend::on_state_file_save_clicked() {
	FileDescriptor::Ptr fd = FileDescriptor::create_open(secondary_controls.state_file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2SPacketType::SAVE_STATE;
	send_packet(packet, fd);
}

AI::BE::Simulator::BackendFactory::BackendFactory() : AI::BE::BackendFactory("Simulator") {
}

void AI::BE::Simulator::BackendFactory::create_backend(const std::multimap<Glib::ustring, Glib::ustring> &params, std::function<void(AI::BE::Backend &)> cb) const {
	std::string load_filename;
	for (std::multimap<Glib::ustring, Glib::ustring>::const_iterator i = params.begin(), iend = params.end(); i != iend; ++i) {
		if (i->first == "load") {
			load_filename = Glib::filename_from_utf8(i->second);
		} else {
			throw std::runtime_error(Glib::ustring::compose("Unrecognized backend parameter %1 (recognized parameters are \"load\")", i->first));
		}
	}
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

