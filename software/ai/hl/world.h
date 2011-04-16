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
					 * Sets the destination for this player.
					 *
					 * \param[in] dest the destination position to move to.
					 *
					 * \param[in] ori the target orientation to assume.
					 *
					 * \param[in] vel the velocity the robot should be moving when it arrives at the target point.
					 */
					virtual void move(Point dest, double ori, Point vel) = 0;

					/**
					 * Returns the movement flags for this player.
					 *
					 * \return the flags governing the movement.
					 */
					virtual unsigned int flags() const = 0;

					/**
					 * Sets the movement flags for this player.
					 *
					 * \param[in] flags the flags governing the movement.
					 */
					virtual void flags(unsigned int flags) = 0;

					/**
					 * Returns the movement type for this player.
					 *
					 * \return the type of movement to perform.
					 */
					virtual AI::Flags::MoveType type() const = 0;

					/**
					 * Sets the movement type for this player.
					 *
					 * \param[in] type the type of movement to perform.
					 */
					virtual void type(AI::Flags::MoveType type) = 0;

					/**
					 * Returns the movement priority for this player.
					 *
					 * \return the priority of the movement.
					 */
					virtual AI::Flags::MovePrio prio() const = 0;

					/**
					 * Sets the movement priority for this player.
					 *
					 * \param[in] prio the priority of the movement.
					 */
					virtual void prio(AI::Flags::MovePrio prio) = 0;

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
					 *
					 * \deprecated in favour of the individual functions for setting different values.
					 */
					void move(Point dest, double ori, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) __attribute__((deprecated));

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
					 *
					 * \deprecated in favour of the individual functions for setting different values.
					 */
					virtual void move(Point dest, double ori, Point vel, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) __attribute__((deprecated)) = 0;

					/**
					 * Causes the player to kick the ball.
					 *
					 * \param[in] speed the speed of the kick, in m/s.
					 */
					virtual void kick(double speed) = 0;
					
					/**
					 * Causes the player to automatically kick the ball as soon as it is picked up by the sensor.
					 *
					 * This function must be called on every tick in order to remain armed; failing to invoke the function will disarm the mechanism.
					 *
					 * \param[in] speed the speed of the kick, in m/s.
					 */
					virtual void autokick(double speed) = 0;
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

