#ifndef AI_BACKEND_SIMULATOR_TEAM_H
#define AI_BACKEND_SIMULATOR_TEAM_H

#include "ai/backend/backend.h"
#include "ai/backend/simulator/player.h"
#include "simulator/sockproto/proto.h"
#include "util/box.h"
#include "util/box_ptr.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <limits>

namespace AI {
	namespace BE {
		namespace Simulator {
			class Backend;

			/**
			 * \brief A general simulator-based team, whether friendly or enemy.
			 *
			 * \tparam T the type of robot held in this team.
			 */
			template<typename T> class GenericTeam : public NonCopyable {
				public:
					/**
					 * \brief Creates a robot.
					 *
					 * \tparam Args the types of the constructor arguments
					 *
					 * \param[in] pattern the pattern index of the robot.
					 *
					 * \param[in] args the constructor arguments
					 */
					template<typename ... Args> void create(unsigned int pattern, Args ... args) {
						assert(pattern < members.size());
						members[pattern].create(args...);
						populate_pointers();
						emit_membership_changed();
					}

					/**
					 * \brief Destroys a robot.
					 *
					 * \param[in] pattern the pattern index to destroy.
					 */
					void destroy(unsigned int pattern) {
						assert(pattern < members.size());
						members[pattern].destroy();
						populate_pointers();
						emit_membership_changed();
					}

					/**
					 * \brief Emits the membership-changed signal.
					 */
					virtual void emit_membership_changed() const = 0;

					std::size_t size() const { return member_ptrs.size(); }
					typename T::Ptr get(std::size_t i) const { return member_ptrs[i]; }

				private:
					/**
					 * \brief The members of the team.
					 */
					std::array<Box<T>, 16> members;
					std::vector<BoxPtr<T>> member_ptrs;

					/**
					 * \brief Rebuilds the member_ptrs array.
					 */
					void populate_pointers() {
						member_ptrs.clear();
						for (Box<T> &i : members) {
							if (i) {
								member_ptrs.push_back(i.ptr());
							}
						}
					}
			};

			/**
			 * The team containing players that the AI can control.
			 */
			class FriendlyTeam : public AI::BE::Team<AI::BE::Player>, public GenericTeam<Player> {
				public:
					/**
					 * Constructs a new FriendlyTeam.
					 *
					 * \param[in] be the backend under which the team lives.
					 */
					explicit FriendlyTeam(Backend &be) : be(be) {
					}

					/**
					 * Retrieves a Player from the team.
					 *
					 * \param[in] i the index of the Player to retrieve.
					 *
					 * \return the Player.
					 */
					Player::Ptr get_impl(std::size_t i) const { return GenericTeam<Player>::get(i); }

					/**
					 * Loads new data into the robots and locks the predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] score the team's new score.
					 *
					 * \param[in] ts the timestamp at which the ball was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2APlayerInfo(&state)[::Simulator::Proto::MAX_PLAYERS_PER_TEAM], unsigned int score, const AI::Timestamp &ts) {
						// Record new score.
						this->score = score;

						// Remove any robots that no longer appear.
						for (std::size_t i = 0; i < size(); ++i) {
							bool found = false;
							for (std::size_t j = 0; j < G_N_ELEMENTS(state) && !found; ++j) {
								if (state[j].robot_info.pattern == get(i)->pattern()) {
									found = true;
								}
							}
							if (!found) {
								destroy(get(i)->pattern());
							}
						}

						// Add any newly-created robots.
						for (const ::Simulator::Proto::S2APlayerInfo &i : state) {
							if (i.robot_info.pattern != std::numeric_limits<unsigned int>::max()) {
								bool found = false;
								for (std::size_t j = 0; j < size() && !found; ++j) {
									if (i.robot_info.pattern == get(j)->pattern()) {
										found = true;
									}
								}
								if (!found) {
									create(i.robot_info.pattern, std::ref(be), i.robot_info.pattern);
								}
							}
						}

						// Update positions and lock in predictors for all robots.
						for (const ::Simulator::Proto::S2APlayerInfo &i : state) {
							if (i.robot_info.pattern != std::numeric_limits<unsigned int>::max()) {
								for (std::size_t j = 0; j < size(); ++j) {
									Player::Ptr plr = get_impl(j);
									if (i.robot_info.pattern == plr->pattern()) {
										plr->pre_tick(i, ts);
									}
								}
							}
						}
					}

					/**
					 * Stores the orders computed by the AI into a packet to send to the simulator.
					 *
					 * \param[out] orders the packet to populate.
					 */
					void encode_orders(::Simulator::Proto::A2SPlayerInfo(&orders)[::Simulator::Proto::MAX_PLAYERS_PER_TEAM]) {
						for (std::size_t i = 0; i < G_N_ELEMENTS(orders); ++i) {
							if (i < size()) {
								get_impl(i)->encode_orders(orders[i]);
							} else {
								orders[i].pattern = std::numeric_limits<unsigned int>::max();
							}
						}
					}

					std::size_t size() const { return GenericTeam<Player>::size(); }
					AI::BE::Player::Ptr get(std::size_t i) const { return get_impl(i); }
					void emit_membership_changed() const { signal_membership_changed().emit(); }

				private:
					/**
					 * \brief The backend under which the team lives.
					 */
					Backend &be;
			};

			/**
			 * The team containing tobots that are controlled by another AI.
			 */
			class EnemyTeam : public AI::BE::Team<AI::BE::Robot>, public GenericTeam<Robot> {
				public:
					/**
					 * Constructs a new EnemyTeam.
					 *
					 * \param[in] be the backend under which the team lives.
					 */
					explicit EnemyTeam(Backend &be) : be(be) {
					}

					/**
					 * Retrieves a Robot from the team.
					 *
					 * \param[in] i the index of the Robot to retrieve.
					 *
					 * \return the Robot.
					 */
					Robot::Ptr get_impl(std::size_t i) const { return GenericTeam<Robot>::get(i); }

					/**
					 * Loads new data into the robots and locks the predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] score the team's new score.
					 *
					 * \param[in] ts the timestamp at which the ball was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2ARobotInfo(&state)[::Simulator::Proto::MAX_PLAYERS_PER_TEAM], unsigned int score, const AI::Timestamp &ts) {
						// Record new score.
						this->score = score;

						// Remove any robots that no longer appear.
						for (std::size_t i = 0; i < size(); ++i) {
							bool found = false;
							for (std::size_t j = 0; j < G_N_ELEMENTS(state) && !found; ++j) {
								if (state[j].pattern == get(i)->pattern()) {
									found = true;
								}
							}
							if (!found) {
								destroy(get(i)->pattern());
							}
						}

						// Add any newly-created robots.
						for (const ::Simulator::Proto::S2ARobotInfo &i : state) {
							if (i.pattern != std::numeric_limits<unsigned int>::max()) {
								bool found = false;
								for (std::size_t j = 0; j < size() && !found; ++j) {
									if (i.pattern == get(j)->pattern()) {
										found = true;
									}
								}
								if (!found) {
									create(i.pattern, i.pattern);
								}
							}
						}

						// Update positions and lock in predictors for all robots.
						for (const ::Simulator::Proto::S2ARobotInfo &i : state) {
							if (i.pattern != std::numeric_limits<unsigned int>::max()) {
								for (std::size_t j = 0; j < size(); ++j) {
									AI::BE::Robot::Ptr bot = get_impl(j);
									if (i.pattern == bot->pattern()) {
										bot->add_field_data({i.x, i.y}, Angle::of_radians(i.orientation), ts);
										bot->pre_tick();
										bot->lock_time(ts);
									}
								}
							}
						}
					}

					std::size_t size() const { return GenericTeam<Robot>::size(); }
					AI::BE::Robot::Ptr get(std::size_t i) const { return get_impl(i); }
					void emit_membership_changed() const { signal_membership_changed().emit(); }

				private:
					/**
					 * \brief The backend under which the team lives.
					 */
					Backend &be;
			};
		}
	}
}

#endif

