#include "ai/backend/simulator/backend.h"

AI::BE::Simulator::UIControls::UIControls() : playtype_label("Play type (sim):"), speed_label("Speed:"), speed_normal(speed_group, "Normal"), speed_fast(speed_group, "Fast"), players_label("Players:"), players_hbox(Gtk::BUTTONBOX_SPREAD), players_add("+"), players_remove("−") {
	for (unsigned int pt = 0; pt < AI::Common::PlayType::COUNT; ++pt) {
		playtype_combo.append_text(AI::Common::PlayType::DESCRIPTIONS_GENERIC[pt]);
	}
	playtype_combo.set_active_text(AI::Common::PlayType::DESCRIPTIONS_GENERIC[AI::Common::PlayType::HALT]);
	playtype_combo.set_sensitive(false);

	speed_normal.set_sensitive(false);
	speed_fast.set_sensitive(false);
	speed_hbox.pack_start(speed_normal, Gtk::PACK_EXPAND_WIDGET);
	speed_hbox.pack_start(speed_fast, Gtk::PACK_EXPAND_WIDGET);

	players_add.set_sensitive(false);
	players_remove.set_sensitive(false);
	players_hbox.pack_start(players_add);
	players_hbox.pack_start(players_remove);
}

AI::BE::Simulator::UIControls::~UIControls() {
}

unsigned int AI::BE::Simulator::UIControls::rows() const {
	return 3;
}

void AI::BE::Simulator::UIControls::attach(Gtk::Table &t, unsigned int row) {
	t.attach(playtype_label, 0, 1, row, row + 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(playtype_combo, 1, 3, row, row + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(speed_label, 0, 1, row + 1, row + 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(speed_hbox, 1, 3, row + 1, row + 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(players_label, 0, 1, row + 2, row + 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
	t.attach(players_hbox, 1, 3, row + 2, row + 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
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

	// Send the magic packet with credentials attached.
	iovec iov = { iov_base: const_cast<char *>(SIMULATOR_SOCKET_MAGIC1), iov_len: std::strlen(SIMULATOR_SOCKET_MAGIC1), };
	ucred creds;
	creds.pid = getpid();
	creds.uid = getuid();
	creds.gid = getgid();
	char cmsgbuf[4096];
	msghdr mh = { msg_name: 0, msg_namelen: 0, msg_iov: &iov, msg_iovlen: 1, msg_control: cmsgbuf, msg_controllen: cmsg_space(sizeof(creds)), msg_flags: 0, };
	cmsg_firsthdr(&mh)->cmsg_len = cmsg_len(sizeof(creds));
	cmsg_firsthdr(&mh)->cmsg_level = SOL_SOCKET;
	cmsg_firsthdr(&mh)->cmsg_type = SCM_CREDENTIALS;
	std::memcpy(cmsg_data(cmsg_firsthdr(&mh)), &creds, sizeof(creds));
	if (sendmsg(sock->fd(), &mh, MSG_NOSIGNAL) < 0) {
		throw SystemError("sendmsg", errno);
	}

	// Allow credentials to be received over this socket.
	const int yes = 1;
	if (setsockopt(sock->fd(), SOL_SOCKET, SO_PASSCRED, &yes, sizeof(yes)) < 0) {
		throw SystemError("setsockopt", errno);
	}

	// Receive the magic packet with the simulator's credentials attached.
	char databuf[4096];
	iov.iov_base = databuf;
	iov.iov_len = sizeof(databuf);
	mh.msg_controllen = sizeof(cmsgbuf);
	ssize_t rc;
	if ((rc = recvmsg(sock->fd(), &mh, 0)) <= 0) {
		if (rc < 0) {
			throw SystemError("recvmsg", errno);
		} else {
			throw std::runtime_error("Simulator unexpectedly closed socket");
		}
	}
	if (std::memcmp(databuf, SIMULATOR_SOCKET_MAGIC2, std::strlen(SIMULATOR_SOCKET_MAGIC2)) != 0) {
		throw std::runtime_error("Signature error on simulator socket; are you running the same version of the simulator and the AI?");
	}
	cmsghdr *ch = cmsg_firsthdr(&mh);
	if (!ch || ch->cmsg_level != SOL_SOCKET || ch->cmsg_type != SCM_CREDENTIALS) {
		throw std::runtime_error("Error on simulator socket: No credentials transmitted");
	}
	std::memcpy(&creds, cmsg_data(ch), sizeof(creds));
	if (creds.uid != getuid()) {
		throw std::runtime_error("Error on simulator socket: User ID of simulator does not match user ID of AI process");
	}

	// We're alive.
	return sock;
}

AI::BE::Simulator::Backend::Backend() : sock(connect_to_simulator()), ball_(*this), friendly_(*this), simulator_playtype(AI::Common::PlayType::HALT) {
	monotonic_time_.tv_sec = 0;
	monotonic_time_.tv_nsec = 0;
	controls.playtype_combo.signal_changed().connect(sigc::mem_fun(this, &Backend::on_sim_playtype_changed));
	controls.speed_normal.signal_toggled().connect(sigc::mem_fun(this, &Backend::on_speed_toggled));
	controls.speed_fast.signal_toggled().connect(sigc::mem_fun(this, &Backend::on_speed_toggled));
	controls.players_add.signal_clicked().connect(sigc::mem_fun(this, &Backend::on_players_add_clicked));
	controls.players_remove.signal_clicked().connect(sigc::mem_fun(this, &Backend::on_players_remove_clicked));
	Glib::signal_io().connect(sigc::mem_fun(this, &Backend::on_packet), sock->fd(), Glib::IO_IN);
	friendly_.score_prop.signal_changed().connect(signal_score_changed().make_slot());
	enemy_.score_prop.signal_changed().connect(signal_score_changed().make_slot());
	playtype_override().signal_changed().connect(sigc::mem_fun(this, &Backend::update_playtype));
}

AI::BE::Simulator::Backend::~Backend() {
}

void AI::BE::Simulator::Backend::send_packet(const ::Simulator::Proto::A2SPacket &packet) {
	if (send(sock->fd(), &packet, sizeof(packet), MSG_NOSIGNAL) < 0) {
		throw SystemError("send", errno);
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

AI::BE::Simulator::EnemyTeam &AI::BE::Simulator::Backend::enemy_team() {
	return enemy_;
}

timespec AI::BE::Simulator::Backend::monotonic_time() const {
	return monotonic_time_;
}

std::size_t AI::BE::Simulator::Backend::visualizable_num_robots() const {
	return friendly_.size() + enemy_.size();
}

Visualizable::Robot::Ptr AI::BE::Simulator::Backend::visualizable_robot(std::size_t i) {
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

unsigned int AI::BE::Simulator::Backend::ui_controls_table_rows() const {
	return controls.rows();
}

void AI::BE::Simulator::Backend::ui_controls_attach(Gtk::Table &t, unsigned int row) {
	controls.attach(t, row);
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
		case ::Simulator::Proto::S2A_PACKET_TICK:
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
			controls.players_add.set_sensitive(friendly_.size() < ::Simulator::Proto::MAX_PLAYERS_PER_TEAM);
			controls.players_remove.set_sensitive(friendly_.size() > 0);
			return true;

		case ::Simulator::Proto::S2A_PACKET_SPEED_MODE:
			// Update the UI controls.
			if (packet.fast) {
				controls.speed_fast.set_active();
			} else {
				controls.speed_normal.set_active();
			}

			// Make both radio buttons sensitive.
			controls.speed_fast.set_sensitive();
			controls.speed_normal.set_sensitive();
			return true;

		case ::Simulator::Proto::S2A_PACKET_PLAY_TYPE:
			if (packet.playtype < AI::Common::PlayType::COUNT) {
				// Record the master play type.
				simulator_playtype = packet.playtype;

				// Update and make sensitive the master play type combo box.
				controls.playtype_combo.set_active_text(AI::Common::PlayType::DESCRIPTIONS_GENERIC[simulator_playtype]);
				controls.playtype_combo.set_sensitive();

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
	packet.type = ::Simulator::Proto::A2S_PACKET_PLAYERS;
	friendly_.encode_orders(packet.players);
	send_packet(packet);
}

void AI::BE::Simulator::Backend::update_playtype() {
	if (playtype_override() != AI::Common::PlayType::COUNT) {
		playtype_rw() = playtype_override();
	} else {
		playtype_rw() = simulator_playtype;
	}
}

void AI::BE::Simulator::Backend::on_sim_playtype_changed() {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2S_PACKET_PLAY_TYPE;
	packet.playtype = AI::Common::PlayType::COUNT;
	for (unsigned int i = 0; i < AI::Common::PlayType::COUNT; ++i) {
		if (controls.playtype_combo.get_active_text() == AI::Common::PlayType::DESCRIPTIONS_GENERIC[i]) {
			packet.playtype = static_cast<AI::Common::PlayType::PlayType>(i);
		}
	}
	send_packet(packet);
}

void AI::BE::Simulator::Backend::on_speed_toggled() {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = controls.speed_fast.get_active() ? ::Simulator::Proto::A2S_PACKET_FAST : ::Simulator::Proto::A2S_PACKET_NORMAL_SPEED;
	send_packet(packet);
}

void AI::BE::Simulator::Backend::on_players_add_clicked() {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2S_PACKET_ADD_PLAYER;
	for (packet.pattern = 0; pattern_exists(packet.pattern); ++packet.pattern) {
	}
	send_packet(packet);
	controls.players_add.set_sensitive(false);
	controls.players_remove.set_sensitive(false);
}

void AI::BE::Simulator::Backend::on_players_remove_clicked() {
	::Simulator::Proto::A2SPacket packet;
	std::memset(&packet, 0, sizeof(packet));
	packet.type = ::Simulator::Proto::A2S_PACKET_REMOVE_PLAYER;
	for (packet.pattern = 0; !pattern_exists(packet.pattern); ++packet.pattern) {
	}
	send_packet(packet);
	controls.players_add.set_sensitive(false);
	controls.players_remove.set_sensitive(false);
}

bool AI::BE::Simulator::Backend::pattern_exists(unsigned int pattern) {
	for (std::size_t i = 0; i < friendly_.size(); ++i) {
		if (friendly_.get(i)->pattern() == pattern) {
			return true;
		}
	}
	return false;
}

AI::BE::Simulator::BackendFactory::BackendFactory() : AI::BE::BackendFactory("Simulator") {
}

AI::BE::Simulator::BackendFactory::~BackendFactory() {
}

void AI::BE::Simulator::BackendFactory::create_backend(const Config &, sigc::slot<void, AI::BE::Backend &> cb) const {
	Backend be;
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

