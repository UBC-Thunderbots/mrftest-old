#include "ai/backend/backend.h"
#include "ai/backend/clock/monotonic.h"
#include "ai/backend/mrf/refbox.h"
#include "ai/backend/physical/player.h"
#include "ai/ball_filter/ball_filter.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "util/box_array.h"
#include "util/codec.h"
#include "util/dprint.h"
#include "util/exception.h"
#include "util/sockaddrs.h"
#include "mrf/dongle.h"
#include "mrf/robot.h"
#include <cassert>
#include <cstring>
#include <locale>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

using namespace AI::BE;

namespace {
	/**
	 * \brief The number of metres the ball must move from a kickoff or similar until we consider that the ball is free to be approached by either team.
	 */
	const double BALL_FREE_DISTANCE = 0.09;

	/**
	 * \brief The number of vision failures to tolerate before assuming the robot is gone and removing it from the system.
	 *
	 * Note that this should be fairly high because the failure count includes instances of a packet arriving from a camera that cannot see the robot
	 * (this is expected to cause a failure to be counted which will then be zeroed out a moment later as the other camera sends its packet).
	 */
	const unsigned int MAX_VISION_FAILURES = 120;

	class MRFBackend;

	/**
	 * \brief A generic team
	 *
	 * \tparam T the type of robot on this team, either Player or Robot
	 *
	 * \tparam TSuper the type of the superclass of the robot, one of the backend Player or Robot classes
	 *
	 * \tparam Super the type of the class's superclass
	 */
	template<typename T, typename TSuper, typename Super> class GenericTeam : public Super {
		public:
			/**
			 * \brief Constructs a new GenericTeam
			 *
			 * \param[in] backend the backend to which the team is attached
			 */
			explicit GenericTeam(MRFBackend &backend);

			/**
			 * \brief Returns the number of existent robots in the team
			 *
			 * \return the number of existent robots in the team
			 */
			std::size_t size() const;

			/**
			 * \brief Returns a robot
			 *
			 * \param[in] i the index of the robot to fetch
			 *
			 * \return the robot
			 */
			typename TSuper::Ptr get(std::size_t i) const;

			/**
			 * \brief Returns a robot as an MRF robot
			 *
			 * \param[in] i the index of the robot to fetch
			 *
			 * \return the robot
			 */
			typename T::Ptr get_mrf_robot(std::size_t i) const;

			/**
			 * \brief Removes all robots from the team
			 */
			void clear();

			/**
			 * \brief Updates the robots on the team using data from SSL-Vision
			 *
			 * \param[in] packets the packets to extract vision data from
			 *
			 * \param[in] ts the time at which the packet was received
			 */
			void update(const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *packets[2], const timespec &ts);

			/**
			 * \brief Locks a time for prediction across all players on the team
			 *
			 * \param[in] now the time to lock as time zero
			 */
			void lock_time(const timespec &now);

		protected:
			MRFBackend &backend;
			BoxArray<T, 16> members;
			std::vector<typename T::Ptr> member_ptrs;
			std::vector<unsigned int> vision_failures;

			void populate_pointers();
			virtual void create_member(unsigned int pattern) = 0;
	};

	/**
	 * \brief The friendly team.
	 */
	class MRFFriendlyTeam : public GenericTeam<AI::BE::Physical::Player, AI::BE::Player, AI::BE::Team<AI::BE::Player>> {
		public:
			explicit MRFFriendlyTeam(MRFBackend &backend, MRFDongle &dongle);

		protected:
			void create_member(unsigned int pattern);

		private:
			MRFDongle &dongle;
	};

	/**
	 * \brief The enemy team.
	 */
	class MRFEnemyTeam : public GenericTeam<AI::BE::Robot, AI::BE::Robot, AI::BE::Team<AI::BE::Robot>> {
		public:
			explicit MRFEnemyTeam(MRFBackend &backend);

		protected:
			void create_member(unsigned int pattern);
	};

	/**
	 * \brief The backend.
	 */
	class MRFBackend : public Backend {
		public:
			AI::BE::MRF::RefBox refbox;

			explicit MRFBackend(MRFDongle &dongle, unsigned int camera_mask, int multicast_interface);
			BackendFactory &factory() const;
			const Team<AI::BE::Player> &friendly_team() const;
			const Team<AI::BE::Robot> &enemy_team() const;
			unsigned int main_ui_controls_table_rows() const;
			void main_ui_controls_attach(Gtk::Table &, unsigned int);
			unsigned int secondary_ui_controls_table_rows() const;
			void secondary_ui_controls_attach(Gtk::Table &, unsigned int);
			std::size_t visualizable_num_robots() const;
			Visualizable::Robot::Ptr visualizable_robot(std::size_t i) const;
			void mouse_pressed(Point, unsigned int);
			void mouse_released(Point, unsigned int);
			void mouse_exited();
			void mouse_moved(Point);
			timespec monotonic_time() const;

		private:
			unsigned int camera_mask;
			AI::BE::Clock::Monotonic clock;
			MRFFriendlyTeam friendly;
			MRFEnemyTeam enemy;
			const FileDescriptor vision_socket;
			timespec playtype_time;
			Point playtype_arm_ball_position;
			SSL_DetectionFrame detections[2];

			void tick();
			bool on_vision_readable(Glib::IOCondition);
			void on_refbox_packet(const void *data, std::size_t length);
			void update_playtype();
			void update_scores();
			void on_friendly_colour_changed();
			AI::Common::PlayType compute_playtype(AI::Common::PlayType old_pt);
	};

	class MRFBackendFactory : public BackendFactory {
		public:
			explicit MRFBackendFactory();
			void create_backend(const std::string &, unsigned int camera_mask, int multicast_interface, std::function<void(Backend &)> cb) const;
	};
}

MRFBackendFactory mrf_backend_factory_instance;

template<typename T, typename TSuper, typename Super> GenericTeam<T, TSuper, Super>::GenericTeam(MRFBackend &backend) : backend(backend) {
}

template<typename T, typename TSuper, typename Super> std::size_t GenericTeam<T, TSuper, Super>::size() const {
	return member_ptrs.size();
}

template<typename T, typename TSuper, typename Super> typename T::Ptr GenericTeam<T, TSuper, Super>::get_mrf_robot(std::size_t i) const {
	return member_ptrs[i];
}

template<typename T, typename TSuper, typename Super> typename TSuper::Ptr GenericTeam<T, TSuper, Super>::get(std::size_t i) const {
	return member_ptrs[i];
}

template<typename T, typename TSuper, typename Super> void GenericTeam<T, TSuper, Super>::clear() {
	for (std::size_t i = 0; i < members.SIZE; ++i) {
		members.destroy(i);
	}
	member_ptrs.clear();
	Super::signal_membership_changed().emit();
}

template<typename T, typename TSuper, typename Super> void GenericTeam<T, TSuper, Super>::update(const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *packets[2], const timespec &ts) {
	bool membership_changed = false;

	// Update existing robots and create new robots.
	std::vector<bool> used_data[2];
	std::vector<bool> seen_this_frame;
	for (std::size_t i = 0; i < 2; ++i) {
		const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep(*packets[i]);
		used_data[i].resize(static_cast<std::size_t>(rep.size()), false);
		for (std::size_t j = 0; j < static_cast<std::size_t>(rep.size()); ++j) {
			const SSL_DetectionRobot &detbot = rep.Get(static_cast<int>(j));
			if (detbot.has_robot_id()) {
				unsigned int pattern = detbot.robot_id();
				typename T::Ptr bot = members[pattern];
				if (!bot) {
					create_member(pattern);
					membership_changed = true;
				}
				if (seen_this_frame.size() <= bot->pattern()) {
					seen_this_frame.resize(bot->pattern() + 1);
				}
				if (bot && !seen_this_frame[bot->pattern()]) {
					seen_this_frame[bot->pattern()] = true;
					if (detbot.has_orientation()) {
						bool neg = backend.defending_end() == AI::BE::Backend::FieldEnd::EAST;
						Point pos((neg ? -detbot.x() : detbot.x()) / 1000.0, (neg ? -detbot.y() : detbot.y()) / 1000.0);
						Angle ori = (Angle::of_radians(detbot.orientation()) + (neg ? Angle::HALF : Angle::ZERO)).angle_mod();
						bot->add_field_data(pos, ori, ts);
					} else {
						LOG_WARN("Vision packet has robot with no orientation.");
					}
				}
				used_data[i][j] = true;
			}
		}
	}

	// Count failures.
	for (std::size_t i = 0; i < members.SIZE; ++i) {
		typename T::Ptr bot = members[i];
		if (bot) {
			if (vision_failures.size() <= bot->pattern()) {
				vision_failures.resize(bot->pattern() + 1);
			}
			if (!seen_this_frame[bot->pattern()]) {
				++vision_failures[bot->pattern()];
			} else {
				vision_failures[bot->pattern()] = 0;
			}
			seen_this_frame[bot->pattern()] = false;
			if (vision_failures[bot->pattern()] >= MAX_VISION_FAILURES) {
				members.destroy(i);
				membership_changed = true;
			}
		}
	}

	// If membership changed, rebuild the pointer array and emit the signal.
	if (membership_changed) {
		populate_pointers();
		Super::signal_membership_changed().emit();
	}
}

template<typename T, typename TSuper, typename Super> void GenericTeam<T, TSuper, Super>::lock_time(const timespec &now) {
	for (auto i = member_ptrs.begin(), iend = member_ptrs.end(); i != iend; ++i) {
		(*i)->lock_time(now);
	}
}

template<typename T, typename TSuper, typename Super> void GenericTeam<T, TSuper, Super>::populate_pointers() {
	member_ptrs.clear();
	for (std::size_t i = 0; i < members.SIZE; ++i) {
		typename T::Ptr p = members[i];
		if (p) {
			member_ptrs.push_back(p);
		}
	}
}

MRFFriendlyTeam::MRFFriendlyTeam(MRFBackend &backend, MRFDongle &dongle) : GenericTeam<AI::BE::Physical::Player, AI::BE::Player, AI::BE::Team<AI::BE::Player>>(backend), dongle(dongle) {
}

void MRFFriendlyTeam::create_member(unsigned int pattern) {
	if (pattern < 8) {
		members.create(pattern, pattern, std::ref(dongle.robot(pattern)));
	}
}

MRFEnemyTeam::MRFEnemyTeam(MRFBackend &backend) : GenericTeam<AI::BE::Robot, AI::BE::Robot, AI::BE::Team<AI::BE::Robot>>(backend) {
}

void MRFEnemyTeam::create_member(unsigned int pattern) {
	members.create(pattern, pattern);
}

MRFBackend::MRFBackend(MRFDongle &dongle, unsigned int camera_mask, int multicast_interface) : Backend(), refbox(multicast_interface), camera_mask(camera_mask), friendly(*this, dongle), enemy(*this), vision_socket(FileDescriptor::create_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) {
	if (!(1 <= camera_mask && camera_mask <= 3)) {
		throw std::runtime_error("Invalid camera bitmask (must be 1–3)");
	}

	refbox.command.signal_changed().connect(sigc::mem_fun(this, &MRFBackend::update_playtype));
	friendly_colour().signal_changed().connect(sigc::mem_fun(this, &MRFBackend::on_friendly_colour_changed));
	playtype_override().signal_changed().connect(sigc::mem_fun(this, &MRFBackend::update_playtype));
	refbox.goals_yellow.signal_changed().connect(sigc::mem_fun(this, &MRFBackend::update_scores));
	refbox.goals_blue.signal_changed().connect(sigc::mem_fun(this, &MRFBackend::update_scores));
	refbox.signal_packet.connect(sigc::mem_fun(this, &MRFBackend::on_refbox_packet));

	clock.signal_tick.connect(sigc::mem_fun(this, &MRFBackend::tick));

	addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
	AddrInfoSet ai(0, "10002", &hints);

	vision_socket.set_blocking(false);

	const int one = 1;
	if (setsockopt(vision_socket.fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		throw SystemError("setsockopt(SO_REUSEADDR)", errno);
	}

	if (bind(vision_socket.fd(), ai.first()->ai_addr, ai.first()->ai_addrlen) < 0) {
		throw SystemError("bind(:10002)", errno);
	}

	ip_mreqn mcreq;
	mcreq.imr_multiaddr.s_addr = inet_addr("224.5.23.2");
	mcreq.imr_address.s_addr = get_inaddr_any();
	mcreq.imr_ifindex = multicast_interface;
	if (setsockopt(vision_socket.fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcreq, sizeof(mcreq)) < 0) {
		LOG_INFO("Cannot join multicast group 224.5.23.2 for vision data.");
	}

	Glib::signal_io().connect(sigc::mem_fun(this, &MRFBackend::on_vision_readable), vision_socket.fd(), Glib::IO_IN);

	playtype_time = clock.now();
}

BackendFactory &MRFBackend::factory() const {
	return mrf_backend_factory_instance;
}

const AI::BE::Team<AI::BE::Player> &MRFBackend::friendly_team() const {
	return friendly;
}

const AI::BE::Team<AI::BE::Robot> &MRFBackend::enemy_team() const {
	return enemy;
}

unsigned int MRFBackend::main_ui_controls_table_rows() const {
	return 0;
}

void MRFBackend::main_ui_controls_attach(Gtk::Table &, unsigned int) {
}

unsigned int MRFBackend::secondary_ui_controls_table_rows() const {
	return 0;
}

void MRFBackend::secondary_ui_controls_attach(Gtk::Table &, unsigned int) {
}

std::size_t MRFBackend::visualizable_num_robots() const {
	return friendly.size() + enemy.size();
}

Visualizable::Robot::Ptr MRFBackend::visualizable_robot(std::size_t i) const {
	if (i < friendly.size()) {
		return friendly.get(i);
	} else {
		return enemy.get(i - friendly.size());
	}
}

void MRFBackend::mouse_pressed(Point, unsigned int) {
}

void MRFBackend::mouse_released(Point, unsigned int) {
}

void MRFBackend::mouse_exited() {
}

void MRFBackend::mouse_moved(Point) {
}

void MRFBackend::tick() {
	// If the field geometry is not yet valid, do nothing.
	if (!field_.valid()) {
		return;
	}

	// Do pre-AI stuff (locking predictors).
	monotonic_time_ = clock.now();
	ball_.lock_time(monotonic_time_);
	friendly.lock_time(monotonic_time_);
	enemy.lock_time(monotonic_time_);
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		friendly.get_mrf_robot(i)->pre_tick();
	}
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		enemy.get_mrf_robot(i)->pre_tick();
	}

	// Run the AI.
	signal_tick().emit();

	// Do post-AI stuff (pushing data to the radios).
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		friendly.get_mrf_robot(i)->tick(playtype() == AI::Common::PlayType::HALT);
	}

	// Notify anyone interested in the finish of a tick.
	timespec after;
	after = clock.now();
	signal_post_tick().emit(timespec_to_nanos(timespec_sub(after, monotonic_time_)));
}

bool MRFBackend::on_vision_readable(Glib::IOCondition) {
	// Receive a packet.
	uint8_t buffer[65536];
	ssize_t len = recv(vision_socket.fd(), buffer, sizeof(buffer), 0);
	if (len < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			LOG_WARN("Cannot receive packet from SSL-Vision.");
		}
		return true;
	}

	// Decode it.
	SSL_WrapperPacket packet;
	if (!packet.ParseFromArray(buffer, static_cast<int>(len))) {
		LOG_WARN("Received malformed SSL-Vision packet.");
		return true;
	}

	// Pass it to any attached listeners.
	timespec now;
	now = clock.now();
	signal_vision().emit(now, packet);

	// If it contains geometry data, update the field shape.
	if (packet.has_geometry()) {
		const SSL_GeometryData &geom(packet.geometry());
		const SSL_GeometryFieldSize &fsize(geom.field());
		double length = fsize.field_length() / 1000.0;
		double total_length = length + (2.0 * fsize.boundary_width() + 2.0 * fsize.referee_width()) / 1000.0;
		double width = fsize.field_width() / 1000.0;
		double total_width = width + (2.0 * fsize.boundary_width() + 2.0 * fsize.referee_width()) / 1000.0;
		double goal_width = fsize.goal_width() / 1000.0;
		double centre_circle_radius = fsize.center_circle_radius() / 1000.0;
		double defense_area_radius = fsize.defense_radius() / 1000.0;
		double defense_area_stretch = fsize.defense_stretch() / 1000.0;
		field_.update(length, total_length, width, total_width, goal_width, centre_circle_radius, defense_area_radius, defense_area_stretch);
	}

	// If it contains ball and robot data, update the ball and the teams.
	if (packet.has_detection()) {
		const SSL_DetectionFrame &det(packet.detection());

		// Check for a sensible camera ID number.
		if (det.camera_id() >= 2) {
			LOG_WARN(Glib::ustring::compose("Received SSL-Vision packet for unknown camera %1.", det.camera_id()));
			return true;
		}

		// Check for an accepted camera ID number.
		if (!(camera_mask & (1U << det.camera_id()))) {
			return true;
		}

		// Keep a local copy of all detection frames.
		detections[det.camera_id()].CopyFrom(det);

		// Take a timestamp.
		timespec now;
		now = clock.now();

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
				pos = ball_filter()->filter(balls, AI::BF::W::World(*this));
			}

			// Use the result.
			ball_.add_field_data(defending_end() == FieldEnd::EAST ? -pos : pos, now);
		}

		// Update the robots.
		const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *yellow_packets[2] = { &detections[0].robots_yellow(), &detections[1].robots_yellow() };
		const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *blue_packets[2] = { &detections[0].robots_blue(), &detections[1].robots_blue() };
		if (friendly_colour() == AI::Common::Colour::YELLOW) {
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

void MRFBackend::on_refbox_packet(const void *data, std::size_t length) {
	timespec now;
	now = clock.now();
	signal_refbox().emit(now, data, length);
}

void MRFBackend::update_playtype() {
	AI::Common::PlayType new_pt;
	AI::Common::PlayType old_pt = playtype();
	if (playtype_override() != AI::Common::PlayType::NONE) {
		new_pt = playtype_override();
	} else {
		if (friendly_colour() == AI::Common::Colour::YELLOW) {
			old_pt = AI::Common::PlayTypeInfo::invert(old_pt);
		}
		new_pt = compute_playtype(old_pt);
		if (friendly_colour() == AI::Common::Colour::YELLOW) {
			new_pt = AI::Common::PlayTypeInfo::invert(new_pt);
		}
	}
	if (new_pt != playtype()) {
		playtype_rw() = new_pt;
		playtype_time = clock.now();
	}
}

void MRFBackend::update_scores() {
	if (friendly_colour() == AI::Common::Colour::YELLOW) {
		friendly.score = refbox.goals_yellow.get();
		enemy.score = refbox.goals_blue.get();
	} else {
		friendly.score = refbox.goals_blue.get();
		enemy.score = refbox.goals_yellow.get();
	}
}

void MRFBackend::on_friendly_colour_changed() {
	update_playtype();
	update_scores();
	friendly.clear();
	enemy.clear();
}

AI::Common::PlayType MRFBackend::compute_playtype(AI::Common::PlayType old_pt) {
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

MRFBackendFactory::MRFBackendFactory() : BackendFactory("mrf") {
}

void MRFBackendFactory::create_backend(const std::string &, unsigned int camera_mask, int multicast_interface, std::function<void(Backend &)> cb) const {
	MRFDongle dongle;
	MRFBackend be(dongle, camera_mask, multicast_interface);
	cb(be);
}

