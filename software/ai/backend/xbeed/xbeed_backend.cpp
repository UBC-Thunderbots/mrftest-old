#include "ai/backend/backend.h"
#include "ai/backend/xbeed/ball.h"
#include "ai/backend/xbeed/field.h"
#include "ai/backend/xbeed/player.h"
#include "ai/backend/xbeed/refbox.h"
#include "ai/backend/xbeed/robot.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "util/clocksource_timerfd.h"
#include "util/dprint.h"
#include "util/sockaddrs.h"
#include "util/timestep.h"
#include "xbee/client/drive.h"
#include "xbee/client/lowlevel.h"
#include <cassert>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace AI::BE;

namespace {
	/**
	 * The number of metres the ball must move from a kickoff or similar until we consider that the ball is free to be approached by either team.
	 */
	const double BALL_FREE_DISTANCE = 0.09;

	/**
	 * The number of vision failures to tolerate before assuming the robot is gone and removing it from the system.
	 * Note that this should be fairly high because the failure count includes instances of a packet arriving from a camera that cannot see the robot
	 * (this is expected to cause a failure to be counted which will then be zeroed out a moment later as the other camera sends its packet).
	 */
	const unsigned int MAX_VISION_FAILURES = 120;

	class XBeeDBackend;

	/**
	 * A generic team.
	 *
	 * \tparam T the type of robot on this team, either Player or Robot.
	 */
	template<typename T> class GenericTeam {
		public:
			GenericTeam(XBeeDBackend &backend);
			~GenericTeam();
			void update(const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *packets[2], const timespec &ts);
			void lock_time(const timespec &now);
			virtual std::size_t size() const = 0;

		protected:
			XBeeDBackend &backend;
			std::vector<typename T::Ptr> members;

			virtual typename T::Ptr create_member(unsigned int pattern) = 0;
			virtual sigc::signal<void, std::size_t> &signal_robot_added() const = 0;
			virtual sigc::signal<void, std::size_t> &signal_robot_removed() const = 0;
	};

	template<typename T> GenericTeam<T>::GenericTeam(XBeeDBackend &backend) : backend(backend) {
	}

	template<typename T> GenericTeam<T>::~GenericTeam() {
	}

	/**
	 * The friendly team.
	 */
	class XBeeDFriendlyTeam : public FriendlyTeam, public GenericTeam<AI::BE::XBeeD::Player> {
		public:
			XBeeDFriendlyTeam(XBeeDBackend &backend);
			~XBeeDFriendlyTeam();
			unsigned int score() const;
			std::size_t size() const;
			Player::Ptr get(std::size_t i) { return members[i]; }
			AI::BE::XBeeD::Player::Ptr create_member(unsigned int pattern);
			AI::BE::XBeeD::Player::Ptr get_xbeed_player(std::size_t i) { return members[i]; }

		private:
			XBeeLowLevel modem;
			std::vector<XBeeDriveBot::Ptr> xbee_bots;

			sigc::signal<void, std::size_t> &signal_robot_added() const { return FriendlyTeam::signal_robot_added(); }
			sigc::signal<void, std::size_t> &signal_robot_removed() const { return FriendlyTeam::signal_robot_removed(); }
	};

	/**
	 * The enemy team.
	 */
	class XBeeDEnemyTeam : public EnemyTeam, public GenericTeam<AI::BE::XBeeD::Robot> {
		public:
			XBeeDEnemyTeam(XBeeDBackend &backend);
			~XBeeDEnemyTeam();
			unsigned int score() const;
			std::size_t size() const;
			Robot::Ptr get(std::size_t i) { return members[i]; }
			AI::BE::XBeeD::Robot::Ptr create_member(unsigned int pattern);

		private:
			sigc::signal<void, std::size_t> &signal_robot_added() const { return EnemyTeam::signal_robot_added(); }
			sigc::signal<void, std::size_t> &signal_robot_removed() const { return EnemyTeam::signal_robot_removed(); }
	};

	/**
	 * The backend.
	 */
	class XBeeDBackend : public Backend {
		public:
			const Config &conf;
			AI::BE::XBeeD::RefBox refbox;

			XBeeDBackend(const Config &conf) : Backend(), conf(conf), clock(UINT64_C(1000000000) / TIMESTEPS_PER_SECOND), ball_(*this), friendly(*this), enemy(*this), vision_socket(FileDescriptor::create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {
				refbox.command.signal_changed().connect(sigc::mem_fun(this, &XBeeDBackend::update_playtype));
				friendly_colour().signal_changed().connect(sigc::mem_fun(this, &XBeeDBackend::update_playtype));

				clock.signal_tick.connect(sigc::mem_fun(this, &XBeeDBackend::tick));

				vision_socket->set_blocking(false);
				const int one = 1;
				if (setsockopt(vision_socket->fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
					throw std::runtime_error("Cannot set SO_REUSEADDR.");
				}
				SockAddrs sa;
				sa.in.sin_family = AF_INET;
				sa.in.sin_addr.s_addr = get_inaddr_any();
				sa.in.sin_port = htons(10002);
				std::memset(sa.in.sin_zero, 0, sizeof(sa.in.sin_zero));
				if (bind(vision_socket->fd(), &sa.sa, sizeof(sa.in)) < 0) {
					throw std::runtime_error("Cannot bind to port 10002 for vision data.");
				}
				ip_mreqn mcreq;
				mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.2");
				mcreq.imr_address.s_addr = get_inaddr_any();
				mcreq.imr_ifindex = 0;
				if (setsockopt(vision_socket->fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
					LOG_INFO("Cannot join multicast group 224.5.23.2 for vision data.");
				}
				Glib::signal_io().connect(sigc::mem_fun(this, &XBeeDBackend::on_vision_readable), vision_socket->fd(), Glib::IO_IN);

				timespec_now(playtype_time);
			}

			~XBeeDBackend() {
			}

			const Field &field() const {
				return field_;
			}

			const Ball &ball() const {
				return ball_;
			}

			FriendlyTeam &friendly_team() {
				return friendly;
			}

			EnemyTeam &enemy_team() {
				return enemy;
			}

			std::size_t visualizable_num_robots() const {
				return friendly.size() + enemy.size();
			}

			Visualizable::Robot::Ptr visualizable_robot(std::size_t i) {
				if (i < friendly.size()) {
					return friendly.get(i);
				} else {
					return enemy.get(i - friendly.size());
				}
			}

		private:
			TimerFDClockSource clock;
			AI::BE::XBeeD::Field field_;
			AI::BE::XBeeD::Ball ball_;
			XBeeDFriendlyTeam friendly;
			XBeeDEnemyTeam enemy;
			const FileDescriptor::Ptr vision_socket;
			timespec playtype_time;
			Point playtype_arm_ball_position;
			SSL_DetectionFrame detections[2];

			void tick() {
				// Do pre-AI stuff (locking predictors).
				timespec now;
				timespec_now(now);
				ball_.lock_time(now);
				friendly.lock_time(now);
				enemy.lock_time(now);

				// Run the AI.
				signal_tick().emit();

				// Do post-AI stuff (pushing data to the dæmon).
				for (unsigned int i = 0; i < friendly.size(); ++i) {
					friendly.get_xbeed_player(i)->tick(playtype() == AI::Common::PlayType::HALT);
				}
			}

			bool on_vision_readable(Glib::IOCondition) {
				// Receive a packet.
				uint8_t buffer[65536];
				ssize_t len = recv(vision_socket->fd(), buffer, sizeof(buffer), 0);
				if (len < 0) {
					if (errno != EAGAIN && errno != EWOULDBLOCK) {
						LOG_WARN("Cannot receive packet from SSL-Vision.");
					}
					return true;
				}

				// Decode it.
				SSL_WrapperPacket packet;
				if (!packet.ParseFromArray(buffer, len)) {
					LOG_WARN("Received malformed SSL-Vision packet.");
					return true;
				}

				// If it contains geometry data, update the field shape.
				if (packet.has_geometry()) {
					const SSL_GeometryData &geom(packet.geometry());
					const SSL_GeometryFieldSize &fsize(geom.field());
					field_.update(fsize);
				}

				// If it contains ball and robot data, update the ball and the teams.
				if (packet.has_detection()) {
					const SSL_DetectionFrame &det(packet.detection());

					// Check for a sensible camera ID number.
					if (det.camera_id() >= 2) {
						LOG_WARN(Glib::ustring::compose("Received SSL-Vision packet for unknown camera %1.", det.camera_id()));
						return true;
					}

					// Keep a local copy of all detection frames.
					detections[det.camera_id()].CopyFrom(det);

					// Take a timestamp.
					timespec now;
					timespec_now(now);

					// Update the ball.
					{
						// Build a vector of all detections so far.
						std::vector<std::pair<double, Point> > balls;
						for (unsigned int i = 0; i < 2; ++i) {
							for (int j = 0; j < detections[i].balls_size(); ++j) {
								const SSL_DetectionBall &b(detections[i].balls(j));
								balls.push_back(std::make_pair(b.confidence(), Point(b.x() / 1000.0, b.y() / 1000.0)));
							}
						}

						// Execute the current ball filter.
						Point pos;
						if (ball_filter()) {
							pos = ball_filter()->filter(balls, *this);
						}

						// Use the result.
						ball_.update(pos, now);
					}

					// Update the robots.
					const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *yellow_packets[2] = { &detections[0].robots_yellow(), &detections[1].robots_yellow() };
					const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *blue_packets[2] = { &detections[0].robots_blue(), &detections[1].robots_blue() };
					if (friendly_colour() == YELLOW) {
						friendly.update(yellow_packets, now);
						enemy.update(blue_packets, now);
					} else {
						friendly.update(blue_packets, now);
						enemy.update(yellow_packets, now);
					}
				}

				// Movement of the ball may, potentially, result in a play type change.
				update_playtype();

				return true;
			}

			void update_playtype() {
				AI::Common::PlayType::PlayType old_pt = playtype();
				if (friendly_colour() == YELLOW) {
					old_pt = AI::Common::PlayType::INVERT[old_pt];
				}
				AI::Common::PlayType::PlayType new_pt = compute_playtype(old_pt);
				if (friendly_colour() == YELLOW) {
					new_pt = AI::Common::PlayType::INVERT[new_pt];
				}
				if (new_pt != playtype()) {
					playtype_rw() = new_pt;
					timespec_now(playtype_time);
				}
			}

			AI::Common::PlayType::PlayType compute_playtype(AI::Common::PlayType::PlayType old_pt) {
				if (playtype_override() != AI::Common::PlayType::COUNT) {
					return playtype_override();
				}

				switch (refbox.command) {
					case 'H': // HALT
					case 'h': // HALF TIME
					case 't': // TIMEOUT YELLOW
					case 'T': // TIMEOUT BLUE
						return AI::Common::PlayType::HALT;

					case 'S': // STOP
					case 'z': // END TIMEOUT
						return AI::Common::PlayType::STOP;

					case ' ': // NORMAL START
						switch (old_pt) {
							case AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY:
								playtype_arm_ball_position = ball_.position();
								return AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY;

							case AI::Common::PlayType::PREPARE_KICKOFF_ENEMY:
								playtype_arm_ball_position = ball_.position();
								return AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY;

							case AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY:
								playtype_arm_ball_position = ball_.position();
								return AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY;

							case AI::Common::PlayType::PREPARE_PENALTY_ENEMY:
								playtype_arm_ball_position = ball_.position();
								return AI::Common::PlayType::EXECUTE_PENALTY_ENEMY;

							case AI::Common::PlayType::EXECUTE_KICKOFF_FRIENDLY:
							case AI::Common::PlayType::EXECUTE_KICKOFF_ENEMY:
							case AI::Common::PlayType::EXECUTE_PENALTY_FRIENDLY:
							case AI::Common::PlayType::EXECUTE_PENALTY_ENEMY:
								if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
									return AI::Common::PlayType::PLAY;
								} else {
									return old_pt;
								}

							default:
								return AI::Common::PlayType::PLAY;
						}

					case 'f': // DIRECT FREE KICK YELLOW
						if (old_pt == AI::Common::PlayType::PLAY) {
							return AI::Common::PlayType::PLAY;
						} else if (old_pt == AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) {
							if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
								return AI::Common::PlayType::PLAY;
							} else {
								return AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;
							}
						} else {
							playtype_arm_ball_position = ball_.position();
							return AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;
						}

					case 'F': // DIRECT FREE KICK BLUE
						if (old_pt == AI::Common::PlayType::PLAY) {
							return AI::Common::PlayType::PLAY;
						} else if (old_pt == AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) {
							if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
								return AI::Common::PlayType::PLAY;
							} else {
								return AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;
							}
						} else {
							playtype_arm_ball_position = ball_.position();
							return AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;
						}

					case 'i': // INDIRECT FREE KICK YELLOW
						if (old_pt == AI::Common::PlayType::PLAY) {
							return AI::Common::PlayType::PLAY;
						} else if (old_pt == AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY) {
							if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
								return AI::Common::PlayType::PLAY;
							} else {
								return AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;
							}
						} else {
							playtype_arm_ball_position = ball_.position();
							return AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;
						}

					case 'I': // INDIRECT FREE KICK BLUE
						if (old_pt == AI::Common::PlayType::PLAY) {
							return AI::Common::PlayType::PLAY;
						} else if (old_pt == AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY) {
							if ((ball_.position() - playtype_arm_ball_position).len() > BALL_FREE_DISTANCE) {
								return AI::Common::PlayType::PLAY;
							} else {
								return AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;
							}
						} else {
							playtype_arm_ball_position = ball_.position();
							return AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;
						}

					case 's': // FORCE START
						return AI::Common::PlayType::PLAY;

					case 'k': // KICKOFF YELLOW
						return AI::Common::PlayType::PREPARE_KICKOFF_ENEMY;

					case 'K': // KICKOFF BLUE
						return AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY;

					case 'p': // PENALTY YELLOW
						return AI::Common::PlayType::PREPARE_PENALTY_ENEMY;

					case 'P': // PENALTY BLUE
						return AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY;

					case '1': // BEGIN FIRST HALF
					case '2': // BEGIN SECOND HALF
					case 'o': // BEGIN OVERTIME 1
					case 'O': // BEGIN OVERTIME 2
					case 'a': // BEGIN PENALTY SHOOTOUT
					case 'g': // GOAL YELLOW
					case 'G': // GOAL BLUE
					case 'd': // REVOKE GOAL YELLOW
					case 'D': // REVOKE GOAL BLUE
					case 'y': // YELLOW CARD YELLOW
					case 'Y': // YELLOW CARD BLUE
					case 'r': // RED CARD YELLOW
					case 'R': // RED CARD BLUE
					case 'c': // CANCEL
					default:
						return old_pt;
				}
			}
	};

	template<typename T> void GenericTeam<T>::update(const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *packets[2], const timespec &ts) {
		// Update existing robots.
		std::vector<bool> used_data[2];
		for (unsigned int i = 0; i < 2; ++i) {
			const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep(*packets[i]);
			used_data[i].resize(rep.size(), false);
			for (int j = 0; j < rep.size(); ++j) {
				const SSL_DetectionRobot &detbot = rep.Get(j);
				if (detbot.has_robot_id()) {
					unsigned int pattern = detbot.robot_id();
					for (unsigned int k = 0; k < size(); ++k) {
						typename T::Ptr bot = members[k];
						if (bot->pattern() == pattern) {
							if (!bot->seen_this_frame) {
								bot->seen_this_frame = true;
								bot->update(detbot, ts);
							}
							used_data[i][j] = true;
						}
					}
				}
			}
		}

		// Count failures.
		for (std::size_t i = 0; i < size(); ++i) {
			typename T::Ptr bot = members[i];
			if (!bot->seen_this_frame) {
				++bot->vision_failures;
			} else {
				bot->vision_failures = 0;
			}
			bot->seen_this_frame = false;
			if (bot->vision_failures >= MAX_VISION_FAILURES) {
				bot->object_store().clear();
				bot.reset();
				members.erase(members.begin() + i);
				signal_robot_removed().emit(i);
				--i;
			}
		}

		// Look for new robots and create them.
		for (unsigned int i = 0; i < 2; ++i) {
			const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep(*packets[i]);
			for (int j = 0; j < rep.size(); ++j) {
				if (!used_data[i][j]) {
					const SSL_DetectionRobot &detbot(rep.Get(j));
					if (detbot.has_robot_id()) {
						typename T::Ptr bot = create_member(detbot.robot_id());
						if (bot.is()) {
							unsigned int k;
							for (k = 0; k < members.size() && members[k]->pattern() < detbot.robot_id(); ++k) {
							}
							members.insert(members.begin() + k, bot);
							signal_robot_added().emit(k);
						}
					}
				}
			}
		}
	}

	template<typename T> void GenericTeam<T>::lock_time(const timespec &now) {
		for (typename std::vector<typename T::Ptr>::const_iterator i = members.begin(), iend = members.end(); i != iend; ++i) {
			(*i)->lock_time(now);
		}
	}

	XBeeDFriendlyTeam::XBeeDFriendlyTeam(XBeeDBackend &backend) : GenericTeam<AI::BE::XBeeD::Player>(backend) {
		for (unsigned int i = 0; i < backend.conf.robots().size(); ++i) {
			const Config::RobotInfo &info = backend.conf.robots()[i];
			xbee_bots.push_back(XBeeDriveBot::create(info.pattern, info.address, modem));
		}
	}

	XBeeDFriendlyTeam::~XBeeDFriendlyTeam() {
	}

	unsigned int XBeeDFriendlyTeam::score() const {
		return backend.friendly_colour() == Backend::YELLOW ? backend.refbox.goals_yellow : backend.refbox.goals_blue;
	}

	std::size_t XBeeDFriendlyTeam::size() const {
		return members.size();
	}

	AI::BE::XBeeD::Player::Ptr XBeeDFriendlyTeam::create_member(unsigned int pattern) {
		for (unsigned int i = 0; i < backend.conf.robots().size(); ++i) {
			const Config::RobotInfo &info = backend.conf.robots()[i];
			if (info.pattern == pattern) {
				return AI::BE::XBeeD::Player::create(backend, pattern, xbee_bots[i]);
			}
		}
		return AI::BE::XBeeD::Player::Ptr();
	}

	XBeeDEnemyTeam::XBeeDEnemyTeam(XBeeDBackend &backend) : GenericTeam<AI::BE::XBeeD::Robot>(backend) {
	}

	XBeeDEnemyTeam::~XBeeDEnemyTeam() {
	}

	unsigned int XBeeDEnemyTeam::score() const {
		return backend.friendly_colour() == Backend::YELLOW ? backend.refbox.goals_blue : backend.refbox.goals_yellow;
	}

	std::size_t XBeeDEnemyTeam::size() const {
		return members.size();
	}

	AI::BE::XBeeD::Robot::Ptr XBeeDEnemyTeam::create_member(unsigned int pattern) {
		return AI::BE::XBeeD::Robot::create(backend, pattern);
	}

	class XBeeDBackendFactory : public BackendFactory {
		public:
			XBeeDBackendFactory() : BackendFactory("xbeed") {
			}

			~XBeeDBackendFactory() {
			}

			void create_backend(const Config &conf, sigc::slot<void, Backend &> cb) const {
				XBeeDBackend be(conf);
				cb(be);
			}
	};

	XBeeDBackendFactory factory_instance;
}

