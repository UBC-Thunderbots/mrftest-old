#ifndef AI_BACKEND_SSL_VISION_BACKEND_H
#define AI_BACKEND_SSL_VISION_BACKEND_H

#include "ai/backend/backend.h"
#include "ai/backend/refbox.h"
#include "ai/backend/clock/monotonic.h"
#include "ai/backend/ssl_vision/vision_socket.h"
#include "ai/common/playtype.h"
#include "geom/point.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "util/dprint.h"
#include "util/param.h"
#include "util/time.h"
#include <cmath>
#include <tuple>
#include <utility>
#include <vector>

namespace AI {
	namespace BE {
		namespace SSLVision {
			/**
			 * \brief The minimum probability above which the best ball detection will be accepted.
			 */
			extern DoubleParam BALL_FILTER_THRESHOLD;

			/**
			 * \brief A backend whose input comes from SSL-Vision and the Referee Box (or another tool that uses the same protocol).
			 *
			 * \tparam FriendlyTeam the type of the friendly team.
			 *
			 * \tparam EnemyTeam the type of the enemy team.
			 */
			template<typename FriendlyTeam, typename EnemyTeam> class Backend : public AI::BE::Backend {
				public:
					/**
					 * \brief The number of metres the ball must move from a kickoff or similar until we consider that the ball is free to be approached by either team.
					 */
					static const double BALL_FREE_DISTANCE;

					/**
					 * \brief Constructs a new SSL-Vision-based backend.
					 *
					 * \param[in] disable_cameras a bitmask indicating which cameras should be ignored
					 *
					 * \param[in] multicast_interface the index of the network interface on which to join multicast groups.
					 *
					 * \param[in] vision_port the port on which SSL-Vision data is delivered.
					 */
					explicit Backend(const std::vector<bool> &disable_cameras, int multicast_interface, const std::string &vision_port);

					virtual FriendlyTeam &friendly_team() = 0;
					const FriendlyTeam &friendly_team() const = 0;
					virtual EnemyTeam &enemy_team() = 0;
					const EnemyTeam &enemy_team() const = 0;

				private:
					const std::vector<bool> &disable_cameras;
					AI::BE::RefBox refbox;
					AI::BE::Clock::Monotonic clock;
					AI::BE::SSLVision::VisionSocket vision_rx;
					timespec playtype_time;
					Point playtype_arm_ball_position;
					std::vector<std::pair<SSL_DetectionFrame, timespec>> detections;

					void tick();
					void handle_vision_packet(const SSL_WrapperPacket &packet);
					void on_refbox_packet();
					void update_playtype();
					void update_goalies();
					void update_scores();
					void on_friendly_colour_changed();
					AI::Common::PlayType compute_playtype(AI::Common::PlayType old_pt);
			};
		}
	}
}



template<typename FriendlyTeam, typename EnemyTeam> const double AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>::BALL_FREE_DISTANCE = 0.09;

template<typename FriendlyTeam, typename EnemyTeam> inline AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>::Backend(const std::vector<bool> &disable_cameras, int multicast_interface, const std::string &vision_port) : disable_cameras(disable_cameras), refbox(multicast_interface), vision_rx(multicast_interface, vision_port) {
	friendly_colour().signal_changed().connect(sigc::mem_fun(this, &Backend::on_friendly_colour_changed));
	playtype_override().signal_changed().connect(sigc::mem_fun(this, &Backend::update_playtype));
	refbox.signal_packet.connect(sigc::mem_fun(this, &Backend::on_refbox_packet));

	clock.signal_tick.connect(sigc::mem_fun(this, &Backend::tick));

	vision_rx.signal_vision_data.connect(sigc::mem_fun(this, &Backend::handle_vision_packet));

	playtype_time = clock.now();
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>::tick() {
	// If the field geometry is not yet valid, do nothing.
	if (!field_.valid()) {
		return;
	}

	// Do pre-AI stuff (locking predictors).
	monotonic_time_ = clock.now();
	ball_.lock_time(monotonic_time_);
	friendly_team().lock_time(monotonic_time_);
	enemy_team().lock_time(monotonic_time_);
	for (std::size_t i = 0; i < friendly_team().size(); ++i) {
		friendly_team().get_backend_robot(i)->pre_tick();
	}
	for (std::size_t i = 0; i < enemy_team().size(); ++i) {
		enemy_team().get_backend_robot(i)->pre_tick();
	}

	// Run the AI.
	signal_tick().emit();

	// Do post-AI stuff (pushing data to the radios and updating predictors).
	for (std::size_t i = 0; i < friendly_team().size(); ++i) {
		friendly_team().get_backend_robot(i)->tick(playtype() == AI::Common::PlayType::HALT, playtype() == AI::Common::PlayType::STOP);
		friendly_team().get_backend_robot(i)->update_predictor(monotonic_time_);
	}

	// Notify anyone interested in the finish of a tick.
	timespec after;
	after = clock.now();
	signal_post_tick().emit(timespec_to_nanos(timespec_sub(after, monotonic_time_)));
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>::handle_vision_packet(const SSL_WrapperPacket &packet) {
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

		// Drop packets we are ignoring.
		if (disable_cameras.size() > det.camera_id() && disable_cameras[det.camera_id()]) {
			return;
		}

		// Keep a local copy of all detection frames with timestamps.
		if (detections.size() <= det.camera_id()) {
			detections.resize(det.camera_id() + 1);
		}
		detections[det.camera_id()].first.CopyFrom(det);
		detections[det.camera_id()].second = clock.now();

		// Update the ball.
		{
			// Compute the best ball position from the list of detections.
			bool found = false;
			double best_prob = 0.0;
			Point best_pos;
			timespec best_time;
			for (auto &i : detections) {
				// Estimate the ball’s position at the camera frame’s timestamp.
				double time_delta = timespec_to_double(timespec_sub(i.second, ball_.lock_time()));
				Point estimated_position = ball_.position(time_delta);
				Point estimated_stdev = ball_.position_stdev(time_delta);
				for (int j = 0; j < i.first.balls_size(); ++j) {
					// Compute the probability of this ball being the wanted one.
					const SSL_DetectionBall &b(i.first.balls(j));
					Point detection_position(b.x() / 1000.0, b.y() / 1000.0);
					if (defending_end() == FieldEnd::EAST) {
						detection_position = -detection_position;
					}
					Point distance_from_estimate = detection_position - estimated_position;
					double x_prob = 1.0f / (std::pow(distance_from_estimate.x / estimated_stdev.x, 2.0) + 1.0f);
					double y_prob = 1.0f / (std::pow(distance_from_estimate.y / estimated_stdev.y, 2.0) + 1.0f);
					double prob = x_prob * y_prob * b.confidence();
					if (prob > best_prob || !found) {
						found = true;
						best_prob = prob;
						best_pos = detection_position;
						best_time = i.second;
					}
				}
			}

			// Keep the detection if it is good enough.
			if (best_prob >= BALL_FILTER_THRESHOLD) {
				ball_.add_field_data(best_pos, best_time);
			}
		}

		// Update the robots.
		std::vector<const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *> yellow_packets(detections.size());
		std::vector<const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *> blue_packets(detections.size());
		std::vector<timespec> packet_timestamps;
		for (std::size_t i = 0; i < detections.size(); i++) {
			yellow_packets[i] = &detections[i].first.robots_yellow();
			blue_packets[i] = &detections[i].first.robots_blue();
			packet_timestamps.push_back(detections[i].second);
		}

		if (friendly_colour() == AI::Common::Colour::YELLOW) {
			friendly_team().update(yellow_packets, packet_timestamps);
			enemy_team().update(blue_packets, packet_timestamps);
		} else {
			friendly_team().update(blue_packets, packet_timestamps);
			enemy_team().update(yellow_packets, packet_timestamps);
		}
	}

	// Movement of the ball may, potentially, result in a play type change.
	update_playtype();

	return;
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>::on_refbox_packet() {
	update_goalies();
	update_scores();
	update_playtype();
	timespec now;
	now = clock.now();
	signal_refbox().emit(now, refbox.packet);
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>::update_playtype() {
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

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>::update_goalies() {
	if (friendly_colour() == AI::Common::Colour::YELLOW) {
		friendly_team().goalie = refbox.packet.yellow().goalie();
		enemy_team().goalie = refbox.packet.blue().goalie();
	} else {
		friendly_team().goalie = refbox.packet.blue().goalie();
		enemy_team().goalie = refbox.packet.yellow().goalie();
	}
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>::update_scores() {
	if (friendly_colour() == AI::Common::Colour::YELLOW) {
		friendly_team().score = refbox.packet.yellow().score();
		enemy_team().score = refbox.packet.blue().score();
	} else {
		friendly_team().score = refbox.packet.blue().score();
		enemy_team().score = refbox.packet.yellow().score();
	}
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>::on_friendly_colour_changed() {
	update_playtype();
	update_scores();
	friendly_team().clear();
	enemy_team().clear();
}

template<typename FriendlyTeam, typename EnemyTeam> inline AI::Common::PlayType AI::BE::SSLVision::Backend<FriendlyTeam, EnemyTeam>::compute_playtype(AI::Common::PlayType old_pt) {
	switch (refbox.packet.command()) {
		case SSL_Referee::HALT:
		case SSL_Referee::TIMEOUT_YELLOW:
		case SSL_Referee::TIMEOUT_BLUE:
			return AI::Common::PlayType::HALT;

		case SSL_Referee::STOP:
		case SSL_Referee::GOAL_YELLOW:
		case SSL_Referee::GOAL_BLUE:
			return AI::Common::PlayType::STOP;

		case SSL_Referee::NORMAL_START:
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

		case SSL_Referee::DIRECT_FREE_YELLOW:
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

		case SSL_Referee::DIRECT_FREE_BLUE:
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

		case SSL_Referee::INDIRECT_FREE_YELLOW:
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

		case SSL_Referee::INDIRECT_FREE_BLUE:
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

		case SSL_Referee::FORCE_START:
			return AI::Common::PlayType::PLAY;

		case SSL_Referee::PREPARE_KICKOFF_YELLOW:
			return AI::Common::PlayType::PREPARE_KICKOFF_ENEMY;

		case SSL_Referee::PREPARE_KICKOFF_BLUE:
			return AI::Common::PlayType::PREPARE_KICKOFF_FRIENDLY;

		case SSL_Referee::PREPARE_PENALTY_YELLOW:
			return AI::Common::PlayType::PREPARE_PENALTY_ENEMY;

		case SSL_Referee::PREPARE_PENALTY_BLUE:
			return AI::Common::PlayType::PREPARE_PENALTY_FRIENDLY;
	}

	return old_pt;
}

#endif

