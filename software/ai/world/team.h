#ifndef AI_WORLD_TEAM_H
#define AI_WORLD_TEAM_H

#include "ai/world/player.h"
#include "ai/world/robot.h"
#include "util/noncopyable.h"
#include <cassert>
#include <cstddef>
#include <vector>
#include <sigc++/sigc++.h>

namespace AI {
	class World;

	/**
	 * A Team is a collection of robots.
	 */
	class Team : public NonCopyable {
		public:
			/**
			 * Fired when a Robot is added to the team.
			 */
			mutable sigc::signal<void, unsigned int, Robot::Ptr> signal_robot_added;

			/**
			 * Fired when a Robot is removed from the team.
			 */
			mutable sigc::signal<void, unsigned int, Robot::Ptr> signal_robot_removed;

			/**
			 * Gets the size of the team.
			 *
			 * \return the number of robots on the team.
			 */
			virtual std::size_t size() const = 0;

			/**
			 * Gets a robot from the team.
			 *
			 * \param[in] index the index of the robot to fetch.
			 *
			 * \return the robot.
			 */
			virtual Robot::Ptr get_robot(unsigned int index) const = 0;

			/**
			 * Converts the team into a \c vector.
			 *
			 * \return all the \ref Robot "Robots" on the team.
			 */
			virtual std::vector<Robot::Ptr> get_robots() const = 0;

		protected:
			/**
			 * Constructs a new Team.
			 */
			Team();

			/**
			 * Removes a robot from the team.
			 *
			 * \param[in] index the index of the robot to remove.
			 */
			virtual void remove(unsigned int index) = 0;

		private:
			friend class World;
	};

	/**
	 * An EnemyTeam is a collection of robots that cannot be driven.
	 */
	class EnemyTeam : public Team {
		public:
			std::size_t size() const {
				return members.size();
			}

			Robot::Ptr get_robot(unsigned int index) const {
				assert(index < size());
				return members[index];
			}

			/**
			 * Gets an enemy robot.
			 *
			 * \param[in] index the index of the robot to fetch.
			 *
			 * \return the robot.
			 */
			Robot::Ptr operator[](unsigned int index) const {
				assert(index < size());
				return members[index];
			}

			std::vector<Robot::Ptr> get_robots() const {
				return members;
			}

		private:
			std::vector<Robot::Ptr> members;

			/**
			 * Adds a robot to the team.
			 *
			 * \param[in] bot the robot to add.
			 */
			void add(Robot::Ptr bot);

			/**
			 * Removes a robot from the team.
			 *
			 * \param[in] index the index of the robot to remove.
			 */
			void remove(unsigned int index);

			friend class World;
	};

	/**
	 * A FriendlyTeam is a collection of players that can be driven.
	 */
	class FriendlyTeam : public Team {
		public:
			/**
			 * Fired when a Player is added to the team. Using this signal instead
			 * of \ref signal_robot_added avoids the need to cast the Robot::Ptr to
			 * a Player::Ptr.
			 */
			mutable sigc::signal<void, unsigned int, Player::Ptr> signal_player_added;

			/**
			 * Fired when a Player is removed from the team. Using this signal
			 * instead of \ref signal_robot_removed avoids the need to cast the
			 * Robot::Ptr to a Player::Ptr.
			 */
			mutable sigc::signal<void, unsigned int, Player::Ptr> signal_player_removed;

			std::size_t size() const {
				return members.size();
			}

			Robot::Ptr get_robot(unsigned int index) const {
				return get_player(index);
			}

			/**
			 * Gets a Player from this team. Using this function instead of
			 * get_robot(unsigned int) const avoids the need to cast the returned
			 * Robot::Ptr to a Player::Ptr.
			 *
			 * \param[in] index the index of the player to fetch.
			 *
			 * \return the player.
			 */
			Player::Ptr get_player(unsigned int index) const {
				assert(index < size());
				return members[index];
			}

			/**
			 * Gets a Player from this team.
			 *
			 * \param[in] index the index of the player to fetch.
			 *
			 * \return the player.
			 */
			Player::Ptr operator[](unsigned int index) const {
				assert(index < size());
				return members[index];
			}

			/**
			 * Converts the FriendlyTeam into a \c vector.
			 *
			 * \return all the \ref Player "Players" on the team.
			 */
			const std::vector<Player::Ptr>& get_players() const {
				return members;
			}

			std::vector<Robot::Ptr> get_robots() const {
				return std::vector<Robot::Ptr>(members.begin(), members.end());
			}

		private:
			std::vector<Player::Ptr> members;

			/**
			 * Adds a player to the team.
			 *
			 * \param[in] bot the player to add.
			 */
			void add(Player::Ptr bot);

			/**
			 * Removes a player from the team.
			 *
			 * \param[in] index the index of the robot to remove.
			 */
			void remove(unsigned int index);

			friend class World;
	};
}

#endif

