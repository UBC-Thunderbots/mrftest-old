#ifndef AI_NAVIGATOR_WORLD_H
#define AI_NAVIGATOR_WORLD_H

#include "ai/common/world.h"
#include "ai/flags.h"
#include "util/byref.h"
#include <utility>
#include <vector>

namespace AI {
	namespace Nav {
		namespace W {
			/**
			 * The field, as seen by a Navigator.
			 */
			class Field : public AI::Common::Field {
			};

			/**
			 * The ball, as seen by a Navigator.
			 */
			class Ball : public AI::Common::Ball {
			};

			/**
			 * A robot, as seen by a Navigator.
			 */
			class Robot : public AI::Common::Robot {
				public:
					/**
					 * A pointer to a Robot.
					 */
					typedef RefPtr<Robot> Ptr;
			};

			/**
			 * A player, as seen by a Navigator.
			 */
			class Player : public Robot, public AI::Common::Player {
				public:
					/**
					 * A pointer to a Player.
					 */
					typedef RefPtr<Player> Ptr;

					/**
					 * Returns the destination position and orientation requested by the Strategy.
					 *
					 * \return the destination position and orientation.
					 */
					virtual const std::pair<Point, double> &destination() const = 0;

					/**
					 * Returns the movement flags requested by the Strategy.
					 *
					 * \return the flags.
					 */
					virtual unsigned int flags() const = 0;

					/**
					 * Returns the movement type requested by the Strategy.
					 *
					 * \return the type.
					 */
					virtual AI::Flags::MOVE_TYPE type() const = 0;

					/**
					 * Returns the movement priority requested by the Strategy.
					 *
					 * \return the priority.
					 */
					virtual AI::Flags::MOVE_PRIO prio() const = 0;

					/**
					 * Sets the path this player should follow.
					 *
					 * \param[in] p the path, in the form of a set of
					 * ((<var>position</var>, <var>orientation</var>), <var>deadline</var>) pairs,
					 * where <var>deadline</var> is the timestamp, in monotonic time, at which the robot should arrive.
					 */
					virtual void path(const std::vector<std::pair<std::pair<Point, double>, timespec> > &p) = 0;
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
						return get_navigator_player(i);
					}

				protected:
					/**
					 * Returns a player from the team.
					 *
					 * \param[in] i the index of the player.
					 *
					 * \return the player.
					 */
					virtual Player::Ptr get_navigator_player(std::size_t i) = 0;
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
					Robot::Ptr get(std::size_t i) {
						return get_navigator_robot(i);
					}

				protected:
					/**
					 * Returns a robot from the team.
					 *
					 * \param[in] i the index of the robot.
					 *
					 * \return the robot.
					 */
					virtual Robot::Ptr get_navigator_robot(std::size_t i) = 0;
			};

			/**
			 * The world, as seen by a Navigator.
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
					 * Returns the enemy team.
					 *
					 * \return the enemy team.
					 */
					virtual EnemyTeam &enemy_team() = 0;

					/**
					 * Returns the current monotonic time.
					 * Monotonic time is a way of representing "game time", which always moves forward.
					 * Monotonic time is consistent within the game world, and may or may not be linked to real time.
					 * A navigator should \em always use this function to retrieve monotonic time, not one of the functions in util/time.h!
					 * The AI will not generally have any use for real time.
					 *
					 * \return the current monotonic time.
					 */
					virtual timespec monotonic_time() const = 0;
			};
		}
	}
}

#endif

