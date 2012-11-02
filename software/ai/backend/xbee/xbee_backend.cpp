#include "ai/backend/backend.h"
#include "ai/backend/refbox.h"
#include "ai/backend/ssl_vision.h"
#include "ai/backend/clock/monotonic.h"
#include "ai/backend/physical/player.h"
#include "ai/backend/physical/team.h"
#include "ai/ball_filter/ball_filter.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "util/dprint.h"
#include "xbee/dongle.h"
#include "xbee/robot.h"

using namespace AI::BE;

namespace {
	/**
	 * \brief The number of metres the ball must move from a kickoff or similar until we consider that the ball is free to be approached by either team.
	 */
	const double BALL_FREE_DISTANCE = 0.09;

	/**
	 * \brief The friendly team.
	 */
	class FriendlyTeam : public AI::BE::Physical::Team<AI::BE::Physical::Player, AI::BE::Player> {
		public:
			explicit FriendlyTeam(Backend &backend, XBeeDongle &dongle);

		protected:
			void create_member(unsigned int pattern);

		private:
			XBeeDongle &dongle;
	};

	/**
	 * \brief The enemy team.
	 */
	class EnemyTeam : public AI::BE::Physical::Team<AI::BE::Robot, AI::BE::Robot> {
		public:
			explicit EnemyTeam(Backend &backend);

		protected:
			void create_member(unsigned int pattern);
	};

	/**
	 * \brief The backend.
	 */
	class XBeeBackend : public Backend {
		public:
			AI::BE::RefBox refbox;

			explicit XBeeBackend(XBeeDongle &dongle, unsigned int camera_mask, int multicast_interface);
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
			FriendlyTeam friendly;
			EnemyTeam enemy;
			AI::BE::VisionReceiver vision_rx;
			timespec playtype_time;
			Point playtype_arm_ball_position;
			SSL_DetectionFrame detections[2];

			void tick();
			void handle_vision_packet(const SSL_WrapperPacket &packet);
			void on_refbox_packet(const void *data, std::size_t length);
			void update_playtype();
			void update_scores();
			void on_friendly_colour_changed();
			AI::Common::PlayType compute_playtype(AI::Common::PlayType old_pt);
	};

	class XBeeBackendFactory : public BackendFactory {
		public:
			explicit XBeeBackendFactory();
			void create_backend(const std::string &, unsigned int camera_mask, int multicast_interface, std::function<void(Backend &)> cb) const;
	};
}

XBeeBackendFactory xbee_backend_factory_instance;

FriendlyTeam::FriendlyTeam(Backend &backend, XBeeDongle &dongle) : AI::BE::Physical::Team<AI::BE::Physical::Player, AI::BE::Player>(backend), dongle(dongle) {
}

void FriendlyTeam::create_member(unsigned int pattern) {
	members.create(pattern, pattern, std::ref(dongle.robot(pattern)));
}

EnemyTeam::EnemyTeam(Backend &backend) : AI::BE::Physical::Team<AI::BE::Robot, AI::BE::Robot>(backend) {
}

void EnemyTeam::create_member(unsigned int pattern) {
	members.create(pattern, pattern);
}

XBeeBackend::XBeeBackend(XBeeDongle &dongle, unsigned int camera_mask, int multicast_interface) : Backend(), refbox(multicast_interface), camera_mask(camera_mask), friendly(*this, dongle), enemy(*this), vision_rx(multicast_interface) {
	if (!(1 <= camera_mask && camera_mask <= 3)) {
		throw std::runtime_error("Invalid camera bitmask (must be 1–3)");
	}

	refbox.command.signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::update_playtype));
	friendly_colour().signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::on_friendly_colour_changed));
	playtype_override().signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::update_playtype));
	refbox.goals_yellow.signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::update_scores));
	refbox.goals_blue.signal_changed().connect(sigc::mem_fun(this, &XBeeBackend::update_scores));
	refbox.signal_packet.connect(sigc::mem_fun(this, &XBeeBackend::on_refbox_packet));

	clock.signal_tick.connect(sigc::mem_fun(this, &XBeeBackend::tick));

	vision_rx.signal_vision_data.connect(sigc::mem_fun(this, &XBeeBackend::handle_vision_packet));

	playtype_time = clock.now();
}

BackendFactory &XBeeBackend::factory() const {
	return xbee_backend_factory_instance;
}

const Team<AI::BE::Player> &XBeeBackend::friendly_team() const {
	return friendly;
}

const Team<AI::BE::Robot> &XBeeBackend::enemy_team() const {
	return enemy;
}

unsigned int XBeeBackend::main_ui_controls_table_rows() const {
	return 0;
}

void XBeeBackend::main_ui_controls_attach(Gtk::Table &, unsigned int) {
}

unsigned int XBeeBackend::secondary_ui_controls_table_rows() const {
	return 0;
}

void XBeeBackend::secondary_ui_controls_attach(Gtk::Table &, unsigned int) {
}

std::size_t XBeeBackend::visualizable_num_robots() const {
	return friendly.size() + enemy.size();
}

Visualizable::Robot::Ptr XBeeBackend::visualizable_robot(std::size_t i) const {
	if (i < friendly.size()) {
		return friendly.get(i);
	} else {
		return enemy.get(i - friendly.size());
	}
}

void XBeeBackend::mouse_pressed(Point, unsigned int) {
}

void XBeeBackend::mouse_released(Point, unsigned int) {
}

void XBeeBackend::mouse_exited() {
}

void XBeeBackend::mouse_moved(Point) {
}

void XBeeBackend::tick() {
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
		friendly.get_backend_robot(i)->pre_tick();
	}
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		enemy.get_backend_robot(i)->pre_tick();
	}

	// Run the AI.
	signal_tick().emit();

	// Do post-AI stuff (pushing data to the radios).
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		friendly.get_backend_robot(i)->tick(playtype() == AI::Common::PlayType::HALT);
	}

	// Notify anyone interested in the finish of a tick.
	timespec after;
	after = clock.now();
	signal_post_tick().emit(timespec_to_nanos(timespec_sub(after, monotonic_time_)));
}

void XBeeBackend::handle_vision_packet(const SSL_WrapperPacket &packet) {
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
			return;
		}

		// Check for an accepted camera ID number.
		if (!(camera_mask & (1U << det.camera_id()))) {
			return;
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

	return;
}

void XBeeBackend::on_refbox_packet(const void *data, std::size_t length) {
	timespec now;
	now = clock.now();
	signal_refbox().emit(now, data, length);
}

void XBeeBackend::update_playtype() {
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

void XBeeBackend::update_scores() {
	if (friendly_colour() == AI::Common::Colour::YELLOW) {
		friendly.score = refbox.goals_yellow.get();
		enemy.score = refbox.goals_blue.get();
	} else {
		friendly.score = refbox.goals_blue.get();
		enemy.score = refbox.goals_yellow.get();
	}
}

void XBeeBackend::on_friendly_colour_changed() {
	update_playtype();
	update_scores();
	friendly.clear();
	enemy.clear();
}

AI::Common::PlayType XBeeBackend::compute_playtype(AI::Common::PlayType old_pt) {
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

XBeeBackendFactory::XBeeBackendFactory() : BackendFactory("xbee") {
}

void XBeeBackendFactory::create_backend(const std::string &, unsigned int camera_mask, int multicast_interface, std::function<void(Backend &)> cb) const {
	XBeeDongle dongle;
	dongle.enable();
	XBeeBackend be(dongle, camera_mask, multicast_interface);
	cb(be);
}

