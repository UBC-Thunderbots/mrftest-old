#ifndef AI_BACKEND_SIMULATOR_TEAM_H
#define AI_BACKEND_SIMULATOR_TEAM_H

#include "ai/backend/backend.h"
#include "ai/backend/simulator/player.h"
#include "ai/backend/simulator/robot.h"
#include "simulator/sockproto/proto.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <limits>

namespace AI {
	namespace BE {
		namespace Simulator {
			/**
			 * A general simulator-based team, whether friendly or enemy.
			 *
			 * \tparam T the type of pointer to robot held in this team.
			 */
			template<typename T> class GenericTeam : public NonCopyable {
				public:
					/**
					 * The property holding the team's score.
					 */
					Property<unsigned int> score_prop;

					/**
					 * Constructs a new GenericTeam.
					 */
					GenericTeam() : score_prop(0) {
					}

					/**
					 * Destroys a GenericTeam.
					 */
					~GenericTeam() {
					}

					/**
					 * Adds a new robot to the team.
					 *
					 * \param[in] bot the robot to add.
					 */
					void add(T bot) {
						members.push_back(bot);
						emit_robot_added(members.size() - 1);
					}

					/**
					 * Removes a robot from the team.
					 *
					 * \param[in] i the index of the robot to remove.
					 */
					void remove(std::size_t i) {
						emit_robot_removing(i);
						members.erase(members.begin() + i);
					}

					unsigned int score() const { return score_prop; }
					std::size_t size() const { return members.size(); }
					T get(std::size_t i) { return members[i]; }

				private:
					/**
					 * The members of the team.
					 */
					std::vector<T> members;

					/**
					 * Emits the robot added signal.
					 *
					 * \param[in] i the index of the new robot.
					 */
					virtual void emit_robot_added(std::size_t i) const = 0;

					/**
					 * Emits the robot removing signal.
					 *
					 * \param[in] i the index of the about-to-be-deleted robot.
					 */
					virtual void emit_robot_removing(std::size_t i) const = 0;
			};

			/**
			 * The team containing \ref Player "Players" that the AI can control.
			 */
			class FriendlyTeam : public AI::BE::FriendlyTeam, public GenericTeam<Player::Ptr> {
				public:
					/**
					 * Constructs a new FriendlyTeam.
					 */
					FriendlyTeam() {
					}

					/**
					 * Destroys a FriendlyTeam.
					 */
					~FriendlyTeam() {
					}

					/**
					 * Retrieves a Player from the team.
					 *
					 * \param[in] i the index of the Player to retrieve.
					 *
					 * \return the Player.
					 */
					Player::Ptr get_impl(std::size_t i) { return GenericTeam<Player::Ptr>::get(i); }

					/**
					 * Loads new data into the robots and locks the predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] score the team's new score.
					 *
					 * \param[in] ts the timestamp at which the ball was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2APlayerInfo (&state)[::Simulator::Proto::MAX_PLAYERS_PER_TEAM], unsigned int score, const timespec &ts) {
						// Record new score.
						score_prop = score;

						// Remove any robots that no longer appear.
						for (std::size_t i = 0; i < size(); ++i) {
							bool found = false;
							for (std::size_t j = 0; j < G_N_ELEMENTS(state) && !found; ++j) {
								if (state[j].robot_info.pattern == get(i)->pattern()) {
									found = true;
								}
							}
							if (!found) {
								remove(i--);
							}
						}

						// Add any newly-created robots.
						for (std::size_t i = 0; i < G_N_ELEMENTS(state); ++i) {
							if (state[i].robot_info.pattern != std::numeric_limits<unsigned int>::max()) {
								bool found = false;
								for (std::size_t j = 0; j < size() && !found; ++j) {
									if (state[i].robot_info.pattern == get(j)->pattern()) {
										found = true;
									}
								}
								if (!found) {
									add(AI::BE::Simulator::Player::create(state[i].robot_info.pattern));
								}
							}
						}

						// Update positions and lock in predictors for all robots.
						for (std::size_t i = 0; i < G_N_ELEMENTS(state); ++i) {
							if (state[i].robot_info.pattern != std::numeric_limits<unsigned int>::max()) {
								for (std::size_t j = 0; j < size(); ++j) {
									Player::Ptr plr = get_impl(j);
									if (state[i].robot_info.pattern == plr->pattern()) {
										plr->pre_tick(state[i], ts);
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
					void encode_orders(::Simulator::Proto::A2SPlayerInfo (&orders)[::Simulator::Proto::MAX_PLAYERS_PER_TEAM]) {
						for (std::size_t i = 0; i < G_N_ELEMENTS(orders); ++i) {
							if (i < size()) {
								get_impl(i)->encode_orders(orders[i]);
							} else {
								orders[i].pattern = std::numeric_limits<unsigned int>::max();
							}
						}
					}

					unsigned int score() const { return GenericTeam<Player::Ptr>::score(); }
					std::size_t size() const { return GenericTeam<Player::Ptr>::size(); }
					AI::BE::Player::Ptr get(std::size_t i) { return get_impl(i); }
					void emit_robot_added(std::size_t i) const { signal_robot_added().emit(i); }
					void emit_robot_removing(std::size_t i) const { signal_robot_removing().emit(i); }
			};

			/**
			 * The team containing \ref Robot "Robots" that are controlled by another AI.
			 */
			class EnemyTeam : public AI::BE::EnemyTeam, public GenericTeam<Robot::Ptr> {
				public:
					/**
					 * Constructs a new EnemyTeam.
					 */
					EnemyTeam() {
					}

					/**
					 * Destroys a EnemyTeam.
					 */
					~EnemyTeam() {
					}

					/**
					 * Retrieves a Robot from the team.
					 *
					 * \param[in] i the index of the Robot to retrieve.
					 *
					 * \return the Robot.
					 */
					Robot::Ptr get_impl(std::size_t i) { return GenericTeam<Robot::Ptr>::get(i); }

					/**
					 * Loads new data into the robots and locks the predictors.
					 *
					 * \param[in] state the state block sent by the simulator.
					 *
					 * \param[in] score the team's new score.
					 *
					 * \param[in] ts the timestamp at which the ball was in this position.
					 */
					void pre_tick(const ::Simulator::Proto::S2ARobotInfo (&state)[::Simulator::Proto::MAX_PLAYERS_PER_TEAM], unsigned int score, const timespec &ts) {
						// Record new score.
						score_prop = score;

						// Remove any robots that no longer appear.
						for (std::size_t i = 0; i < size(); ++i) {
							bool found = false;
							for (std::size_t j = 0; j < G_N_ELEMENTS(state) && !found; ++j) {
								if (state[j].pattern == get(i)->pattern()) {
									found = true;
								}
							}
							if (!found) {
								remove(i--);
							}
						}

						// Add any newly-created robots.
						for (std::size_t i = 0; i < G_N_ELEMENTS(state); ++i) {
							if (state[i].pattern != std::numeric_limits<unsigned int>::max()) {
								bool found = false;
								for (std::size_t j = 0; j < size() && !found; ++j) {
									if (state[i].pattern == get(j)->pattern()) {
										found = true;
									}
								}
								if (!found) {
									add(AI::BE::Simulator::Robot::create(state[i].pattern));
								}
							}
						}

						// Update positions and lock in predictors for all robots.
						for (std::size_t i = 0; i < G_N_ELEMENTS(state); ++i) {
							if (state[i].pattern != std::numeric_limits<unsigned int>::max()) {
								for (std::size_t j = 0; j < size(); ++j) {
									Robot::Ptr bot = get_impl(j);
									if (state[i].pattern == bot->pattern()) {
										bot->pre_tick(state[i], ts);
									}
								}
							}
						}
					}

					unsigned int score() const { return GenericTeam<Robot::Ptr>::score(); }
					std::size_t size() const { return GenericTeam<Robot::Ptr>::size(); }
					AI::BE::Robot::Ptr get(std::size_t i) { return get_impl(i); }
					void emit_robot_added(std::size_t i) const { signal_robot_added().emit(i); }
					void emit_robot_removing(std::size_t i) const { signal_robot_removing().emit(i); }
			};
		}
	}
}

#endif

