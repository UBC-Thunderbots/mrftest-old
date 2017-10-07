#ifndef AI_BACKEND_SSL_VISION_BACKEND_H
#define AI_BACKEND_SSL_VISION_BACKEND_H

#include "ai/backend/backend.h"
#include "ai/backend/refbox.h"
#include "ai/backend/clock/monotonic.h"
#include "ai/backend/vision/vision_socket.h"
#include "ai/common/playtype.h"
#include "geom/point.h"
#include "geom/particle/particle_filter.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "util/dprint.h"
#include "util/param.h"
#include <chrono>
#include <cmath>
#include <tuple>
#include <utility>
#include <vector>
#include <iostream>

namespace AI {
namespace BE {
namespace Vision {
/**
 *
 */
extern BoolParam DISABLE_VISION_FILTER;

extern BoolParam USE_PARTICLE_FILTER;

/**
 * \brief The minimum probability above which the best ball detection will be accepted.
 */
extern DoubleParam BALL_FILTER_THRESHOLD;

/**
 * \brief The distance from the centre of the robot to the centre of the ball when touching the dribbler.
 */
constexpr double ROBOT_CENTRE_TO_FRONT_DISTANCE = 0.087;

/**
 * \brief A backend whose input comes from SSL-Vision and the Referee Box (or another tool that uses the same protocol).
 *
 * \tparam FriendlyTeam the type of the friendly team.
 *
 * \tparam EnemyTeam the type of the enemy team.
 */
template<typename FriendlyTeam, typename EnemyTeam> class Backend: public AI::BE::Backend {
public:
	/**
	 * \brief The number of metres the ball must move from a kickoff or similar until we consider that the ball is free to be approached by either team.
	 */
	static constexpr double BALL_FREE_DISTANCE = 0.09;

	/**
	 * \brief The maximum speed that a ball can be to consider kickoff ending.
	 */
	static constexpr double BALL_FREE_MAXSPEED = 8;

	/**
	 * \brief Constructs a new SSL-Vision-based backend.
	 *
	 * \param[in] disable_cameras a bitmask indicating which cameras should be ignored
	 *
	 * \param[in] multicast_interface the index of the network interface on which to join multicast groups.
	 *
	 * \param[in] vision_port the port on which SSL-Vision data is delivered.
	 */
	explicit Backend(const std::vector<bool> &disable_cameras,
			int multicast_interface);

	virtual FriendlyTeam &friendly_team() = 0;
	const FriendlyTeam &friendly_team() const override = 0;
	virtual EnemyTeam &enemy_team() = 0;
	const EnemyTeam &enemy_team() const override = 0;
	void handle_vision_packet(const SSL_WrapperPacket &packet);

private:
	const std::vector<bool> &disable_cameras;
	AI::BE::RefBox refbox;
	AI::BE::Clock::Monotonic clock;
	AI::Timestamp playtype_time;
	Point playtype_arm_ball_position;
	std::vector<std::pair<SSL_DetectionFrame, AI::Timestamp>> detections;
	AI::BE::Vision::Particle::ParticleFilter *pFilter_;

	virtual void tick() = 0;
	void on_refbox_packet();
	void update_geometry(const SSL_GeometryData &geom);
	void update_ball(const SSL_DetectionFrame &det, AI::Timestamp time_rec);
	void update_playtype();
	void update_goalies();
	void update_scores();
	void update_ball_placement();
	void on_friendly_colour_changed();
	AI::Common::PlayType compute_playtype(AI::Common::PlayType old_pt);
};
}
}
}

template<typename FriendlyTeam, typename EnemyTeam> constexpr double AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::BALL_FREE_DISTANCE;

template<typename FriendlyTeam, typename EnemyTeam> inline AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::Backend(
		const std::vector<bool> &disable_cameras, int multicast_interface):
		disable_cameras(disable_cameras), refbox(multicast_interface){
	friendly_colour().signal_changed().connect(
			sigc::mem_fun(this, &Backend::on_friendly_colour_changed));
	playtype_override().signal_changed().connect(
			sigc::mem_fun(this, &Backend::update_playtype));
	refbox.signal_packet.connect(
			sigc::mem_fun(this, &Backend::on_refbox_packet));

	clock.signal_tick.connect(sigc::mem_fun(this, &Backend::tick));

	playtype_time = std::chrono::steady_clock::now();

	pFilter_ = nullptr;
}
/*
template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::tick() {

	// If the field geometry is not yet valid, do nothing.
	if (!field_.valid()) {
		return;
	}

	// Do pre-AI stuff (locking predictors).
	monotonic_time_ = std::chrono::steady_clock::now();
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
		//test to see if this fixes halt spamming over radio
		friendly_team().get_backend_robot(i)->tick(false,false);
		//friendly_team().get_backend_robot(i)->tick(
				//playtype() == AI::Common::PlayType::HALT,
				//playtype() == AI::Common::PlayType::STOP);
		friendly_team().get_backend_robot(i)->update_predictor(monotonic_time_);
	}

	// Notify anyone interested in the finish of a tick.
	AI::Timestamp after;
	after = std::chrono::steady_clock::now();
	signal_post_tick().emit(after - monotonic_time_);
}
*/
template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::handle_vision_packet(
		const SSL_WrapperPacket &packet) {
	AI::Timestamp time_rec;
	time_rec = std::chrono::steady_clock::now();
	// Pass it to any attached listeners.
	signal_vision().emit(time_rec, packet);

	// If it contains geometry data, update the field shape.
	if (packet.has_geometry()) {
		const SSL_GeometryData &geom(packet.geometry());
		update_geometry(geom);
	}

	if (!pFilter_ && field_.valid()) {
		pFilter_ = new AI::BE::Vision::Particle::ParticleFilter(field_.length(),
				field_.width());
	}

	// If it contains ball and robot data, update the ball and the teams.

	if (packet.has_detection() && field_.valid()) {
		const SSL_DetectionFrame &det(packet.detection());

		// Drop packets we are ignoring.
		if (disable_cameras.size() > det.camera_id()
				&& disable_cameras[det.camera_id()]) {
			return;
		}

		// Keep a local copy of all detection frames with timestamps.
		if (detections.size() <= det.camera_id()) {
			detections.resize(det.camera_id() + 1);
		}
		detections[det.camera_id()].first.CopyFrom(det);
		detections[det.camera_id()].second = time_rec;

		// Update the ball.
		update_ball(det, time_rec);

		// Update the robots.
		std::vector<
				const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *> yellow_packets(
				detections.size());
		std::vector<
				const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *> blue_packets(
				detections.size());
		std::vector<AI::Timestamp> packet_timestamps;
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


template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::update_geometry(const SSL_GeometryData &geom) {
	const SSL_GeometryFieldSize &fsize(geom.field());
	const double referee_width = 400;
	double length = fsize.field_length() / 1000.0;
	double total_length = length + (2.0 * fsize.boundary_width() + 2.0 * referee_width)
					/ 1000.0;

	double width = fsize.field_width() / 1000.0;
	double total_width = width + (2.0 * fsize.boundary_width() + 2.0 * referee_width)
					/ 1000.0;

	double goal_width = fsize.goal_width() / 1000.0;

	// Circular arcs
	/*
	Names:
	LeftFieldLeftPenaltyArc
	LeftFieldRightPenaltyArc
	RightFieldLeftPenaltyArc
	RightFieldRightPenaltyArc
	CenterCircle

	*/
	double centre_circle_radius;
	double defense_area_radius;

	double left_penalty_stretch;
	double right_penalty_stretch;
	double defense_area_stretch;

	// Get center circle radius, including line thickness
	for (int i = 0; i < fsize.field_arcs_size(); i++)
	{
		const SSL_FieldCicularArc& arc = fsize.field_arcs(i);
		std::string arcName = arc.name();
		if (arcName == "CenterCircle")
		{
			centre_circle_radius = (arc.radius() + arc.thickness() / 2.0) / 1000.0;
		}

		else if (arcName == "LeftFieldLeftPenaltyArc")
		{
			defense_area_radius = (arc.radius() + arc.thickness() / 2.0) / 1000.0;
		}


	}

	// Defense stretch
	/*
	Line names:
	TopTouchLine
	BottomTouchLine
	LeftGoalLine
	RightGoalLine
	HalfwayLine
	CenterLine
	LeftPenaltyStretch
	RightPenaltyStretch
	RightGoalTopLine
	RightGoalBottomLine
	RightGoalDepthLine
	LeftGoalTopLine
	LeftGoalBottomLine
	LeftGoalDepthLine

	*/
	for (int i = 0; i < fsize.field_lines_size(); i++)
	{
		const SSL_FieldLineSegment& line = fsize.field_lines(i);
		std::string lineName = line.name();
		// std::cout << lineName << std::endl;
		if (lineName == "LeftPenaltyStretch")
		{
			left_penalty_stretch = (Point(line.p2().x(), line.p2().y()) - Point(line.p1().x(), line.p1().y())).len() / 1000;
		}
		
		else if (lineName == "RightPenaltyStretch")
		{
			right_penalty_stretch = (Point(line.p2().x(), line.p2().y()) - Point(line.p1().x(), line.p1().y())).len() / 1000;
		}
	}
	defense_area_stretch  = left_penalty_stretch;
	// std::cout << defense_area_stretch << std::endl;
	// double centre_circle_radius = fsize.center_circle_radius() / 1000.0;
	// double defense_area_radius = fsize.defense_radius() / 1000.0;
	// double defense_area_stretch = fsize.defense_stretch() / 1000.0;
	field_.update(length, total_length, width, total_width, goal_width,
			centre_circle_radius, defense_area_radius,
			defense_area_stretch);
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::update_ball(const SSL_DetectionFrame &det, AI::Timestamp time_rec) {

	if (!DISABLE_VISION_FILTER) {
		// Compute the best ball position from the list of detections.
		Point best_pos;
		double best_conf = 0;
		AI::Timestamp best_time = time_rec;

		// Estimate the ball’s position at the camera frame’s timestamp.
		double time_delta =
				std::chrono::duration_cast<std::chrono::duration<double>>(
						time_rec - ball_.lock_time()).count();

		/* KALMAN VARIABLE DECLARATIONS */
		Point estimated_position = ball_.position(time_delta);
		Point estimated_velocity = ball_.velocity(time_delta);
		Point estimated_stdev = ball_.position_stdev(time_delta);
		double x_prob = 0, y_prob = 0;
		double best_prob = 0;

		if (time_delta >= 0) {
			bool any_ball_inside = false;
			for (const SSL_DetectionBall &b : det.balls()) {
				if (fabs(b.x()) <= field_.length() * 500 && fabs(b.y()) <= field_.width() * 500) {
					any_ball_inside = true;
					break;
				}
			}

			for (const SSL_DetectionBall &b : det.balls()) {
				if ((fabs(b.x()) > field_.length() * 500 || fabs(b.y()) > field_.width() * 500)
						&& any_ball_inside) {
					continue;
				}

				// Compute the probability of this ball being the wanted one.
				Point detection_position(b.x() / 1000.0, b.y() / 1000.0);
				if (defending_end() == FieldEnd::EAST) {
					detection_position = -detection_position;
				}
				std::cout << "Ball detected: " << detection_position << std::endl;
				if (AI::BE::Vision::USE_PARTICLE_FILTER) {
					/* PARTICLE FILTER */
					pFilter_->add(detection_position);
				} else {
					/* KALMAN FORMULAS */

					/* old Kalman formulae
					 * Point distance_from_estimate = detection_position - estimated_position;
					 * x_prob = 1.0f / (std::pow(distance_from_estimate.x / estimated_stdev.x, 2.0) + 1.0f);
					 * y_prob = 1.0f / (std::pow(distance_from_estimate.y / estimated_stdev.y, 2.0) + 1.0f);
					 */

					/* new Kalman formulae */
					double a = (detection_position.x - estimated_position.x)
							/ estimated_stdev.x;
					x_prob = std::exp(-0.5 * a * a);
					a = (detection_position.y - estimated_position.y)
							/ estimated_stdev.y;
					y_prob = std::exp(-0.5 * a * a);

					double prob = x_prob * y_prob * b.confidence();
					if (prob > best_prob) {
						best_prob = prob;
						best_pos = detection_position;
					}

					if (b.confidence() > best_conf) {
						best_conf = b.confidence();
						best_pos = detection_position;
					}
				}
			}
		}

		if (AI::BE::Vision::USE_PARTICLE_FILTER) {
			pFilter_->update(ball_.position(time_delta));
			ball_.add_field_data(pFilter_->getEstimate(), best_time);

			// This might be useful to include in the future
//			// No useful detection from camera; instead, see if a robot has the ball.
//			std::vector<Point> has_ball_inputs;
//			for (std::size_t i = 0; i < friendly_team().size(); ++i) {
//				Player::Ptr player = friendly_team().get(i);
//				if (player->has_ball()) {
//					player->lock_time(time_rec);
//					Point pos = player->position(0);
//					pos += Point::of_angle(player->orientation(0))
//							* ROBOT_CENTRE_TO_FRONT_DISTANCE;
//					has_ball_inputs.push_back(pos);
//				}
//			}
//
//			if (!has_ball_inputs.empty()) {
//				Point avg;
//				for (auto i : has_ball_inputs) {
//					avg += i;
//				}
//				avg /= static_cast<double>(has_ball_inputs.size());
//				ball_.add_field_data(avg, time_rec);
//			}
		} else {
			/* KALMAN - UPDATE BALL */
			if (best_prob >= BALL_FILTER_THRESHOLD) {
				ball_.add_field_data(best_pos, best_time);
			}
		}
	}
	else {
		bool any_ball_inside = false;
		for (const SSL_DetectionBall &b : det.balls()) {
			if (fabs(b.x()) < field_.length() * 500 && fabs(b.y()) < field_.width() * 500) {
				any_ball_inside = true;
				break;
			}
		}

		Point best_pos = Point();
		double best_conf = 0;
		for (const SSL_DetectionBall &b : det.balls()) {
			if ((fabs(b.x()) > field_.length() * 500 || fabs(b.y()) > field_.width() * 500)
					&& any_ball_inside) {
				continue;
			}

			Point detection_position = Point(b.x() / 1000.0, b.y() / 1000.0);

			if (defending_end() == FieldEnd::EAST) {
				detection_position = -detection_position;
			}

			if (b.confidence() > best_conf) {
				best_pos = detection_position;
				best_conf = b.confidence();
			}
		}

		if (best_conf > 0) {
			ball_.add_field_data(best_pos, time_rec);
		}
	}
}
template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::on_refbox_packet() {
	update_goalies();
	update_scores();
	update_playtype();
	update_ball_placement();
	AI::Timestamp now;
	now = std::chrono::steady_clock::now();
	signal_refbox().emit(now, refbox.packet);
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::update_playtype() {
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
		playtype_time = std::chrono::steady_clock::now();
	}
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::update_goalies() {
	if (friendly_colour() == AI::Common::Colour::YELLOW) {
		friendly_team().goalie = refbox.packet.yellow().goalie();
		enemy_team().goalie = refbox.packet.blue().goalie();
	} else {
		friendly_team().goalie = refbox.packet.blue().goalie();
		enemy_team().goalie = refbox.packet.yellow().goalie();
	}
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::update_ball_placement() {
	if (refbox.packet.has_designated_position()) {
		ball_placement_position_rw() = Point(refbox.packet.designated_position().x() / 1000.0, refbox.packet.designated_position().y() / 1000.0);
	}
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::update_scores() {
	if (friendly_colour() == AI::Common::Colour::YELLOW) {
		friendly_team().score = refbox.packet.yellow().score();
		enemy_team().score = refbox.packet.blue().score();
	} else {
		friendly_team().score = refbox.packet.blue().score();
		enemy_team().score = refbox.packet.yellow().score();
	}
}

template<typename FriendlyTeam, typename EnemyTeam> inline void AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::on_friendly_colour_changed() {
	update_playtype();
	update_scores();
	friendly_team().clear();
	enemy_team().clear();
}

template<typename FriendlyTeam, typename EnemyTeam> inline AI::Common::PlayType AI::BE::Vision::Backend<
		FriendlyTeam, EnemyTeam>::compute_playtype(
		AI::Common::PlayType old_pt) {
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
			if ((ball_.position() - playtype_arm_ball_position).len()
					> BALL_FREE_DISTANCE) {
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
		} else if (old_pt
				== AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) {
			if ((ball_.position() - playtype_arm_ball_position).len()
					> BALL_FREE_DISTANCE && ball_.velocity().len() < BALL_FREE_MAXSPEED) {
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
		} else if (old_pt
				== AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) {
			if ((ball_.position() - playtype_arm_ball_position).len()
					> BALL_FREE_DISTANCE && ball_.velocity().len() < BALL_FREE_MAXSPEED) {
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
		} else if (old_pt
				== AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY) {
			if ((ball_.position() - playtype_arm_ball_position).len()
					> BALL_FREE_DISTANCE && ball_.velocity().len() < BALL_FREE_MAXSPEED) {
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
		} else if (old_pt
				== AI::Common::PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY) {
			if ((ball_.position() - playtype_arm_ball_position).len()
					> BALL_FREE_DISTANCE && ball_.velocity().len() < BALL_FREE_MAXSPEED) {
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

	case SSL_Referee::BALL_PLACEMENT_YELLOW:
		return AI::Common::PlayType::BALL_PLACEMENT_ENEMY;

	case SSL_Referee::BALL_PLACEMENT_BLUE:
		return AI::Common::PlayType::BALL_PLACEMENT_FRIENDLY;
	}

	return old_pt;
}

#endif

