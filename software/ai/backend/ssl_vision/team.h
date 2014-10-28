#ifndef AI_BACKEND_SSL_VISION_TEAM_H
#define AI_BACKEND_SSL_VISION_TEAM_H

#include "ai/backend/backend.h"
#include "proto/messages_robocup_ssl_wrapper.pb.h"
#include "util/box.h"
#include "util/dprint.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <vector>

namespace AI {
	namespace BE {
		namespace SSLVision {
			/**
			 * \brief The number of vision failures to tolerate before assuming the robot is gone and removing it from the system.
			 *
			 * Note that this should be fairly high because the failure count includes instances of a packet arriving from a camera that cannot see the robot
			 * (this is expected to cause a failure to be counted which will then be zeroed out a moment later as the other camera sends its packet).
			 */
			constexpr unsigned int MAX_VISION_FAILURES = 120;

			/**
			 * \brief A generic team.
			 *
			 * \tparam T the type of robot on this team, either Player or Robot.
			 *
			 * \tparam TSuper the type of the superclass of the robot, one of the backend Player or Robot classes.
			 */
			template<typename T, typename TSuper> class Team : public AI::BE::Team<TSuper> {
				public:
					/**
					 * \brief The maximum number of patterns on a team.
					 */
					static constexpr std::size_t NUM_PATTERNS = 16;

					/**
					 * \brief Constructs a new Team.
					 *
					 * \param[in] backend the backend to which the team is attached.
					 */
					explicit Team(AI::BE::Backend &backend);

					/**
					 * \brief Returns the number of existent robots in the team.
					 *
					 * \return the number of existent robots in the team.
					 */
					std::size_t size() const override;

					/**
					 * \brief Returns a robot.
					 *
					 * \param[in] i the index of the robot to fetch.
					 *
					 * \return the robot.
					 */
					typename TSuper::Ptr get(std::size_t i) const override;

					/**
					 * \brief Returns a robot.
					 *
					 * \param[in] i the index of the robot to fetch.
					 *
					 * \return the robot.
					 */
					typename T::Ptr get_backend_robot(std::size_t i) const;

					/**
					 * \brief Removes all robots from the team.
					 */
					void clear();

					/**
					 * \brief Updates the robots on the team using data from SSL-Vision.
					 *
					 * \param[in] packets the packets to extract vision data from.
					 *
					 * \param[in] ts the times at which the packets were received.
					 */
					void update(const std::vector<const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *> &packets, const std::vector<AI::Timestamp> &ts);

					/**
					 * \brief Locks a time for prediction across all players on the team.
					 *
					 * \param[in] now the time to lock as time zero.
					 */
					void lock_time(const AI::Timestamp &now);

				protected:
					AI::BE::Backend &backend;
					std::array<Box<T>, NUM_PATTERNS> members;
					std::vector<typename T::Ptr> member_ptrs;
					unsigned int vision_failures[NUM_PATTERNS];

					void populate_pointers();
					virtual void create_member(unsigned int pattern) = 0;
			};
		}
	}
}



template<typename T, typename TSuper> constexpr std::size_t AI::BE::SSLVision::Team<T, TSuper>::NUM_PATTERNS;

template<typename T, typename TSuper> AI::BE::SSLVision::Team<T, TSuper>::Team(AI::BE::Backend &backend) : backend(backend) {
	std::fill_n(vision_failures, NUM_PATTERNS, 0U);
}

template<typename T, typename TSuper> std::size_t AI::BE::SSLVision::Team<T, TSuper>::size() const {
	return member_ptrs.size();
}

template<typename T, typename TSuper> typename T::Ptr AI::BE::SSLVision::Team<T, TSuper>::get_backend_robot(std::size_t i) const {
	return member_ptrs[i];
}

template<typename T, typename TSuper> typename TSuper::Ptr AI::BE::SSLVision::Team<T, TSuper>::get(std::size_t i) const {
	return member_ptrs[i];
}

template<typename T, typename TSuper> void AI::BE::SSLVision::Team<T, TSuper>::clear() {
	std::for_each(members.begin(), members.end(), std::mem_fn(&Box<T>::destroy));
	member_ptrs.clear();
	AI::BE::Team<TSuper>::signal_membership_changed().emit();
}

template<typename T, typename TSuper> void AI::BE::SSLVision::Team<T, TSuper>::update(const std::vector<const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> *> &packets, const std::vector<AI::Timestamp> &ts) {
	bool membership_changed = false;

	// Update existing robots and create new robots.
	bool seen_this_frame[NUM_PATTERNS];
	std::fill_n(seen_this_frame, NUM_PATTERNS, false);
	for (std::size_t i = 0; i < packets.size(); ++i) {
		const google::protobuf::RepeatedPtrField<SSL_DetectionRobot> &rep(*packets[i]);
		for (std::size_t j = 0; j < static_cast<std::size_t>(rep.size()); ++j) {
			const SSL_DetectionRobot &detbot = rep.Get(static_cast<int>(j));
			if (detbot.has_robot_id()) {
				unsigned int pattern = detbot.robot_id();
				if (pattern < NUM_PATTERNS) {
					const typename T::Ptr &bot = members[pattern].ptr();
					if (!bot) {
						create_member(pattern);
						membership_changed = true;
					}
					if (bot && !seen_this_frame[bot->pattern()]) {
						seen_this_frame[bot->pattern()] = true;
						if (detbot.has_orientation()) {
							bool neg = backend.defending_end() == AI::BE::Backend::FieldEnd::EAST;
							Point pos((neg ? -detbot.x() : detbot.x()) / 1000.0, (neg ? -detbot.y() : detbot.y()) / 1000.0);
							Angle ori = (Angle::of_radians(detbot.orientation()) + (neg ? Angle::half() : Angle::zero())).angle_mod();
							bot->add_field_data(pos, ori, ts[i]);
						} else {
							LOG_WARN(u8"Vision packet has robot with no orientation.");
						}
					}
				}
			}
		}
	}

	// Count failures.
	for (Box<T> &i : members) {
		if (i) {
			const typename T::Ptr &bot = i.ptr();
			assert(bot->pattern() < NUM_PATTERNS);
			if (!seen_this_frame[bot->pattern()]) {
				++vision_failures[bot->pattern()];
			} else {
				vision_failures[bot->pattern()] = 0;
			}
			seen_this_frame[bot->pattern()] = false;
			if (vision_failures[bot->pattern()] >= MAX_VISION_FAILURES) {
				i.destroy();
				membership_changed = true;
			}
		}
	}

	// If membership changed, rebuild the pointer array and emit the signal.
	if (membership_changed) {
		populate_pointers();
		AI::BE::Team<TSuper>::signal_membership_changed().emit();
	}
}

template<typename T, typename TSuper> void AI::BE::SSLVision::Team<T, TSuper>::lock_time(const AI::Timestamp &now) {
	std::for_each(member_ptrs.begin(), member_ptrs.end(), std::bind(std::mem_fn(&T::lock_time), std::placeholders::_1, now));
}

template<typename T, typename TSuper> void AI::BE::SSLVision::Team<T, TSuper>::populate_pointers() {
	member_ptrs.clear();
	for (Box<T> &i : members) {
		if (i) {
			member_ptrs.push_back(i.ptr());
		}
	}
}

#endif

