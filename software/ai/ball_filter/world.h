#ifndef AI_BALL_FILTER_WORLD_H
#define AI_BALL_FILTER_WORLD_H

#include "ai/common/world.h"
#include "util/byref.h"

namespace AI {
	namespace BF {
		namespace W {
			/**
			 * The field, as seen by a ball filter.
			 */
			class Field : public AI::Common::Field {
			};

			/**
			 * The ball, as seen by a ball filter.
			 */
			class Ball : public AI::Common::Ball {
			};

			/**
			 * A robot, as seen by a ball filter.
			 */
			class Robot : public AI::Common::Robot {
				public:
					/**
					 * A pointer to a Robot.
					 */
					typedef RefPtr<Robot> Ptr;
			};

			/**
			 * A player, as seen by a ball filter.
			 */
			class Player : public Robot, public AI::Common::Player {
				public:
					/**
					 * A pointer to a Player.
					 */
					typedef RefPtr<Player> Ptr;
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
						return get_ball_filter_player(i);
					}

				protected:
					/**
					 * Returns a player from the team.
					 *
					 * \param[in] i the index of the player.
					 *
					 * \return the player.
					 */
					virtual Player::Ptr get_ball_filter_player(std::size_t i) = 0;
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
						return get_ball_filter_robot(i);
					}

				protected:
					/**
					 * Returns a robot from the team.
					 *
					 * \param[in] i the index of the robot.
					 *
					 * \return the robot.
					 */
					virtual Robot::Ptr get_ball_filter_robot(std::size_t i) = 0;
			};

			/**
			 * The world, as seen by a Coach.
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
			};
		}
	}
}

#endif

