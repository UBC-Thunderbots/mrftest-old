#ifndef AI_HL_WORLD_H
#define AI_HL_WORLD_H

#include "ai/flags.h"
#include "ai/common/world.h"
#include "util/property.h"
#include <sigc++/sigc++.h>

namespace AI {
	namespace HL {
		namespace W {
			namespace PlayType = AI::Common::PlayType;

			/**
			 * The field, as seen by a Strategy.
			 */
			class Field : public AI::Common::Field {
			};

			/**
			 * The ball, as seen by a Strategy.
			 */
			class Ball : public AI::Common::Ball {
			};

			/**
			 * A robot, as seen by a Strategy.
			 */
			typedef AI::Common::Robot Robot;

			/**
			 * A player, as seen by a Strategy.
			 */
			class Player : public Robot, public AI::Common::Player {
				public:
					/**
					 * A pointer to a Player.
					 */
					typedef RefPtr<Player> Ptr;

					/**
					 * A pointer to a const Player.
					 */
					typedef RefPtr<const Player> CPtr;

					/**
					 * Sets the current destination and movement type for this player.
					 *
					 * \param[in] dest the destination position to move to.
					 *
					 * \param[in] ori the target orientation to assume.
					 *
					 * \param[in] flags the flags governing the movement.
					 *
					 * \param[in] type the type of movement to perform.
					 *
					 * \param[in] prio the priority of the movement.
					 */
					void move(Point dest, double ori, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio);

					/**
					 * Sets the current destination and movement type for this player.
					 *
					 * \param[in] dest the destination position to move to.
					 *
					 * \param[in] ori the target orientation to assume.
					 *
					 * \param[in] vel the velocity the robot should be moving when it arrives at the target point.
					 *
					 * \param[in] flags the flags governing the movement.
					 *
					 * \param[in] type the type of movement to perform.
					 *
					 * \param[in] prio the priority of the movement.
					 */
					virtual void move(Point dest, double ori, Point vel, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) = 0;

					/**
					 * Causes the player to kick the ball.
					 *
					 * \param[in] power the power of the kick, from 0 to 1.
					 */
					virtual void kick(double power) = 0;
					
					/**
					 * Causes the player to autokick the ball.
					 *
					 * \param[in] power the power of the kick, from 0 to 1.
					 */
					virtual void autokick(double power) = 0;
			};

			/**
			 * The friendly team.
			 */
			class FriendlyTeam : public AI::Common::Team {
				public:
					/**
					 * Returns a player from the team.
					 *
					 * \param[in] i the index of the player.
					 *
					 * \return the player.
					 */
					Player::Ptr get(std::size_t i) {
						return get_hl_player(i);
					}

					/**
					 * Returns a player from the team.
					 *
					 * \param[in] i the index of the player.
					 *
					 * \return the player.
					 */
					Player::CPtr get(std::size_t i) const {
						return get_hl_player(i);
					}

				protected:
					/**
					 * Returns a player from the team.
					 *
					 * \param[in] i the index of the player.
					 *
					 * \return the player.
					 */
					virtual Player::Ptr get_hl_player(std::size_t i) = 0;

					/**
					 * Returns a player from the team.
					 *
					 * \param[in] i the index of the player.
					 *
					 * \return the player.
					 */
					virtual Player::CPtr get_hl_player(std::size_t i) const = 0;
			};

			/**
			 * The enemy team.
			 */
			class EnemyTeam : public AI::Common::Team {
				public:
					/**
					 * Returns a robot from the team.
					 *
					 * \param[in] i the index of the robot.
					 *
					 * \return the robot.
					 */
					Robot::Ptr get(std::size_t i) const {
						return get_hl_robot(i);
					}

				protected:
					/**
					 * Returns a robot from the team.
					 *
					 * \param[in] i the index of the robot.
					 *
					 * \return the robot.
					 */
					virtual Robot::Ptr get_hl_robot(std::size_t i) const = 0;
			};

			/**
			 * The world, as seen by a Strategy.
			 */
			class World {
				public:
					/**
					 * Returns the field.
					 *
					 * \return the field.
					 */
					virtual const Field &field() const = 0;

					/**
					 * Returns the ball.
					 *
					 * \return the ball.
					 */
					virtual const Ball &ball() const = 0;

					/**
					 * Returns the friendly team.
					 *
					 * \return the friendly team.
					 */
					virtual FriendlyTeam &friendly_team() = 0;

					/**
					 * Returns the friendly team.
					 *
					 * \return the friendly team.
					 */
					virtual const FriendlyTeam &friendly_team() const = 0;

					/**
					 * Returns the enemy team.
					 *
					 * \return the enemy team.
					 */
					virtual const EnemyTeam &enemy_team() const = 0;

					/**
					 * Returns the current play type.
					 *
					 * \return the current play type.
					 */
					virtual const Property<PlayType::PlayType> &playtype() const = 0;

				protected:
					/**
					 * Constructs a new World.
					 */
					World();

					/**
					 * Destroys a World.
					 */
					~World();
			};
		}
	}
}

#endif

