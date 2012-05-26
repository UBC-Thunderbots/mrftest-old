#ifndef AI_HL_WORLD_H
#define AI_HL_WORLD_H

#include "ai/flags.h"
#include "ai/common/world.h"
#include "util/box_ptr.h"
#include "util/property.h"

namespace AI {
	namespace HL {
		namespace W {
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
			class Robot : public AI::Common::Robot {
				public:
					/**
					 * \brief A pointer to a const Robot.
					 */
					typedef BoxPtr<const Robot> Ptr;

					/**
					 * \brief Sets the avoidance distance for this robot.
					 *
					 * If this function is not called, the avoidance distance is reset to medium.
					 *
					 * This function has no effect on friendly robots.
					 *
					 * \param[in] dist the avoidance distance.
					 */
					virtual void avoid_distance(AI::Flags::AvoidDistance dist) const = 0;
			};

			/**
			 * A player, as seen by a Strategy.
			 */
			class Player : public Robot, public AI::Common::Player {
				public:
					/**
					 * A pointer to a Player.
					 */
					typedef BoxPtr<Player> Ptr;

					/**
					 * A pointer to a const Player.
					 */
					typedef BoxPtr<const Player> CPtr;

					/**
					 * Sets the destination for this player.
					 *
					 * \param[in] dest the destination position to move to.
					 *
					 * \param[in] ori the target orientation to assume.
					 *
					 * \param[in] vel the velocity the robot should be moving when it arrives at the target point.
					 */
					virtual void move(Point dest, Angle ori, Point vel) = 0;

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

					/**
					 * Causes the player to chip the ball up in the air
					 *
					 * \param[in] power the power of the chip, from 0 to 1 in arbitrary units
					 */
					virtual void chip(double power) = 0;

					/**
					 * Causes the player to automatically chip the ball as soon as it is picked up by the sensor.
					 *
					 * This function must be called on every tick in order to remain armed; failing to invoke the function will disarm the mechanism.
					 *
					 * \param[in] power the power of the chip, from 0 to 1 in arbitrary units
					 */
					virtual void autochip(double power) = 0;
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
					virtual const Property<AI::Common::PlayType> &playtype() const = 0;

				protected:
					/**
					 * Constructs a new World.
					 */
					World();
			};
		}
	}
}



inline AI::HL::W::World::World() = default;

#endif

