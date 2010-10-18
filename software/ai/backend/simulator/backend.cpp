#include "ai/backend/backend.h"
#include "ai/backend/simulator/ball.h"
#include "ai/backend/simulator/field.h"
#include "ai/backend/simulator/player.h"
#include "ai/backend/simulator/robot.h"
#include "ai/backend/simulator/team.h"
#include "simulator/sockproto/proto.h"
#include "util/chdir.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/misc.h"
#include "util/sockaddrs.h"
#include <cerrno>
#include <cstring>
#include <gtkmm.h>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

namespace {
	FileDescriptor::Ptr connect_to_simulator() {
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
		if (sendmsg(sock->fd(), &mh, MSG_EOR | MSG_NOSIGNAL) < 0) {
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

	class UIControls {
		public:
			Gtk::Label playtype_label;
			Gtk::ComboBoxText playtype_combo;
			Gtk::Label speed_label;
			Gtk::HBox speed_hbox;
			Gtk::RadioButtonGroup speed_group;
			Gtk::RadioButton speed_normal, speed_fast;
			Gtk::Label players_label;
			Gtk::HButtonBox players_hbox;
			Gtk::Button players_add, players_remove;

			UIControls() : playtype_label("Play type (sim):"), speed_label("Speed:"), speed_normal(speed_group, "Normal"), speed_fast(speed_group, "Fast"), players_label("Players:"), players_hbox(Gtk::BUTTONBOX_SPREAD), players_add("+"), players_remove("−") {
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

			~UIControls() {
			}

			unsigned int rows() const {
				return 3;
			}

			void attach(Gtk::Table &t, unsigned int row) {
				t.attach(playtype_label, 0, 1, row, row + 1, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				t.attach(playtype_combo, 1, 3, row, row + 1, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				t.attach(speed_label, 0, 1, row + 1, row + 2, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				t.attach(speed_hbox, 1, 3, row + 1, row + 2, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				t.attach(players_label, 0, 1, row + 2, row + 3, Gtk::SHRINK | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
				t.attach(players_hbox, 1, 3, row + 2, row + 3, Gtk::EXPAND | Gtk::FILL, Gtk::SHRINK | Gtk::FILL);
			}
	};

	class SimulatorBackend : public AI::BE::Backend {
		public:
			SimulatorBackend() : sock(connect_to_simulator()), simulator_playtype(AI::Common::PlayType::HALT) {
				monotonic_time_.tv_sec = 0;
				monotonic_time_.tv_nsec = 0;
				controls.playtype_combo.signal_changed().connect(sigc::mem_fun(this, &SimulatorBackend::on_sim_playtype_changed));
				controls.speed_normal.signal_toggled().connect(sigc::mem_fun(this, &SimulatorBackend::on_speed_toggled));
				controls.speed_fast.signal_toggled().connect(sigc::mem_fun(this, &SimulatorBackend::on_speed_toggled));
				controls.players_add.signal_clicked().connect(sigc::mem_fun(this, &SimulatorBackend::on_players_add_clicked));
				controls.players_remove.signal_clicked().connect(sigc::mem_fun(this, &SimulatorBackend::on_players_remove_clicked));
				Glib::signal_io().connect(sigc::mem_fun(this, &SimulatorBackend::on_packet), sock->fd(), Glib::IO_IN);
				friendly_.score_prop.signal_changed().connect(signal_score_changed().make_slot());
				enemy_.score_prop.signal_changed().connect(signal_score_changed().make_slot());
				playtype_override().signal_changed().connect(sigc::mem_fun(this, &SimulatorBackend::update_playtype));
			}

			~SimulatorBackend() {
			}

			AI::BE::BackendFactory &factory() const;
			const AI::BE::Field &field() const { return field_; }
			const AI::BE::Ball &ball() const { return ball_; }
			AI::BE::FriendlyTeam &friendly_team() { return friendly_; }
			AI::BE::EnemyTeam &enemy_team() { return enemy_; }
			timespec monotonic_time() const { return monotonic_time_; }
			std::size_t visualizable_num_robots() const { return friendly_.size() + enemy_.size(); }

			Visualizable::Robot::Ptr visualizable_robot(std::size_t i) {
				if (i < friendly_.size()) {
					return friendly_.get(i);
				} else {
					return enemy_.get(i - friendly_.size());
				}
			}

			unsigned int ui_controls_table_rows() const {
				return controls.rows();
			}

			void ui_controls_attach(Gtk::Table &t, unsigned int row) {
				controls.attach(t, row);
			}

		private:
			FileDescriptor::Ptr sock;
			AI::BE::Simulator::Field field_;
			AI::BE::Simulator::Ball ball_;
			AI::BE::Simulator::FriendlyTeam friendly_;
			AI::BE::Simulator::EnemyTeam enemy_;
			timespec monotonic_time_;
			AI::Common::PlayType::PlayType simulator_playtype;
			UIControls controls;

			bool on_packet(Glib::IOCondition) {
				// Receive a packet.
				Simulator::Proto::S2APacket packet;
				iovec iov = { iov_base: &packet, iov_len: sizeof(packet), };
				msghdr mh = { msg_name: 0, msg_namelen: 0, msg_iov: &iov, msg_iovlen: 1, msg_control: 0, msg_controllen: 0, msg_flags: 0 };
				ssize_t rc;
				if ((rc = recvmsg(sock->fd(), &mh, MSG_DONTWAIT)) <= 0) {
					if (rc < 0) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							return true;
						}
						throw SystemError("recvmsg", errno);
					} else {
						throw std::runtime_error("Simulator unexpectedly closed socket");
					}
				}

				// Decide what to do based on the type code.
				switch (packet.type) {
					case Simulator::Proto::S2A_PACKET_TICK:
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
						controls.players_add.set_sensitive(friendly_.size() < Simulator::Proto::MAX_PLAYERS_PER_TEAM);
						controls.players_remove.set_sensitive(friendly_.size() > 0);
						return true;

					case Simulator::Proto::S2A_PACKET_SPEED_MODE:
						if (packet.fast) {
							controls.speed_fast.set_active();
						} else {
							controls.speed_normal.set_active();
						}
						controls.speed_fast.set_sensitive();
						controls.speed_normal.set_sensitive();
						return true;

					case Simulator::Proto::S2A_PACKET_PLAY_TYPE:
						if (packet.playtype < AI::Common::PlayType::COUNT) {
							simulator_playtype = packet.playtype;
							controls.playtype_combo.set_active_text(AI::Common::PlayType::DESCRIPTIONS_GENERIC[simulator_playtype]);
							controls.playtype_combo.set_sensitive();
							update_playtype();
							return true;
						} else {
							throw std::runtime_error("Simulator sent illegal play type");
						}
				}

				throw std::runtime_error("Simulator sent bad packet type");
			}

			void send_orders() {
				Simulator::Proto::A2SPacket packet;
				std::memset(&packet, 0, sizeof(packet));
				packet.type = Simulator::Proto::A2S_PACKET_PLAYERS;
				friendly_.encode_orders(packet.players);
				send_packet(packet);
			}

			void update_playtype() {
				if (playtype_override() != AI::Common::PlayType::COUNT) {
					playtype_rw() = playtype_override();
				} else {
					playtype_rw() = simulator_playtype;
				}
			}

			void on_sim_playtype_changed() {
				Simulator::Proto::A2SPacket packet;
				std::memset(&packet, 0, sizeof(packet));
				packet.type = Simulator::Proto::A2S_PACKET_PLAY_TYPE;
				packet.playtype = AI::Common::PlayType::COUNT;
				for (unsigned int i = 0; i < AI::Common::PlayType::COUNT; ++i) {
					if (controls.playtype_combo.get_active_text() == AI::Common::PlayType::DESCRIPTIONS_GENERIC[i]) {
						packet.playtype = static_cast<AI::Common::PlayType::PlayType>(i);
					}
				}
				send_packet(packet);
			}

			void on_speed_toggled() {
				Simulator::Proto::A2SPacket packet;
				std::memset(&packet, 0, sizeof(packet));
				packet.type = controls.speed_fast.get_active() ? Simulator::Proto::A2S_PACKET_FAST : Simulator::Proto::A2S_PACKET_NORMAL_SPEED;
				send_packet(packet);
			}

			void on_players_add_clicked() {
				Simulator::Proto::A2SPacket packet;
				std::memset(&packet, 0, sizeof(packet));
				packet.type = Simulator::Proto::A2S_PACKET_ADD_PLAYER;
				for (packet.pattern = 0; pattern_exists(packet.pattern); ++packet.pattern);
				send_packet(packet);
				controls.players_add.set_sensitive(false);
				controls.players_remove.set_sensitive(false);
			}

			void on_players_remove_clicked() {
				Simulator::Proto::A2SPacket packet;
				std::memset(&packet, 0, sizeof(packet));
				packet.type = Simulator::Proto::A2S_PACKET_REMOVE_PLAYER;
				for (packet.pattern = 0; !pattern_exists(packet.pattern); ++packet.pattern);
				send_packet(packet);
				controls.players_add.set_sensitive(false);
				controls.players_remove.set_sensitive(false);
			}

			bool pattern_exists(unsigned int pattern) {
				return find_pattern(pattern) != std::numeric_limits<std::size_t>::max();
			}

			std::size_t find_pattern(unsigned int pattern) {
				for (std::size_t i = 0; i < friendly_.size(); ++i) {
					if (friendly_.get(i)->pattern() == pattern) {
						return i;
					}
				}
				return std::numeric_limits<std::size_t>::max();
			}

			void send_packet(const Simulator::Proto::A2SPacket &packet) {
				if (send(sock->fd(), &packet, sizeof(packet), MSG_EOR | MSG_NOSIGNAL) < 0) {
					throw SystemError("sendmsg", errno);
				}
			}
	};

	class SimulatorBackendFactory : public AI::BE::BackendFactory {
		public:
			SimulatorBackendFactory() : AI::BE::BackendFactory("Simulator") {
			}

			~SimulatorBackendFactory() {
			}

			void create_backend(const Config &, sigc::slot<void, AI::BE::Backend &> cb) const {
				SimulatorBackend be;
				cb(be);
			}
	};

	SimulatorBackendFactory factory_instance;

	AI::BE::BackendFactory &SimulatorBackend::factory() const {
		return factory_instance;
	}
}

