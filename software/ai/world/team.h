#ifndef AI_WORLD_TEAM_H
#define AI_WORLD_TEAM_H

#include "ai/world/player.h"
#include "ai/world/robot.h"
#include "util/byref.h"
#include <cassert>
#include <cstddef>
#include <vector>
#include <sigc++/sigc++.h>

class World;

/**
 * A Team is a collection of robots.
 */
class Team : public ByRef {
	public:
		/**
		 * A pointer to a Team.
		 */
		typedef RefPtr<Team> Ptr;

		/**
		 * Fired when a robot is added to the team.
		 */
		mutable sigc::signal<void, unsigned int, Robot::Ptr> signal_robot_added;

		/**
		 * Fired when a robot is removed from the team.
		 */
		mutable sigc::signal<void, unsigned int, Robot::Ptr> signal_robot_removed;

		/**
		 * \return The number of robots on the team.
		 */
		virtual std::size_t size() const = 0;

		/**
		 * \param index the index of the robot to fetch
		 * \return The robot
		 */
		virtual Robot::Ptr get_robot(unsigned int index) const = 0;

		/**
		 * \return A vector of robots.
		 */
		virtual std::vector<Robot::Ptr> get_robots() const = 0;

	protected:
		/**
		 * Constructs a new Team.
		 */
		Team();

		/**
		 * Removes a robot from the team.
		 * \param index the index of the robot to remove
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
		/**
		 * A pointer to a Team.
		 */
		typedef RefPtr<EnemyTeam> Ptr;

		/**
		 * \return The number of robots on the team
		 */
		std::size_t size() const {
			return members.size();
		}

		/**
		 * \param index the index of the robot to fetch
		 * \return The robot
		 */
		Robot::Ptr get_robot(unsigned int index) const {
			assert(index < size());
			return members[index];
		}

		/**
		 * \param index the index of the robot to fetch
		 * \return The robot
		 */
		Robot::Ptr operator[](unsigned int index) const {
			assert(index < size());
			return members[index];
		}

		/**
		 * \return A vector of robots.
		 */
		std::vector<Robot::Ptr> get_robots() const {
			return members;
		}

	private:
		std::vector<Robot::Ptr> members;

		/**
		 * Creates a new Team.
		 */
		static Ptr create();

		/**
		 * Adds a robot to the team.
		 * \param bot the robot to add
		 */
		void add(Robot::Ptr bot);

		/**
		 * Removes a robot from the team.
		 * \param index the index of the robot to remove
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
		 * A pointer to a Team.
		 */
		typedef RefPtr<FriendlyTeam> Ptr;

		/**
		 * Fired when a player is added to the team.
		 */
		mutable sigc::signal<void, unsigned int, Player::Ptr> signal_player_added;

		/**
		 * Fired when a player is removed from the team.
		 */
		mutable sigc::signal<void, unsigned int, Player::Ptr> signal_player_removed;

		/**
		 * \return The number of robots on the team
		 */
		std::size_t size() const {
			return members.size();
		}

		/**
		 * \param index the index of the robot to fetch
		 * \return The robot
		 */
		Robot::Ptr get_robot(unsigned int index) const {
			return get_player(index);
		}

		/**
		 * \param index the index of the player to fetch
		 * \return The player
		 */
		Player::Ptr get_player(unsigned int index) const {
			assert(index < size());
			return members[index];
		}

		/**
		 * \param index the index of the player to fetch
		 * \return The player
		 */
		Player::Ptr operator[](unsigned int index) const {
			assert(index < size());
			return members[index];
		}

		/**
		 * \return A vector of players.
		 */
		const std::vector<Player::Ptr>& get_players() const {
			return members;
		}

		/**
		 * \return A vector of robots.
		 */
		std::vector<Robot::Ptr> get_robots() const {
			return std::vector<Robot::Ptr>(members.begin(), members.end());
		}

	private:
		std::vector<Player::Ptr> members;

		/**
		 * Creates a new Team.
		 */
		static Ptr create();

		/**
		 * Adds a player to the team.
		 * \param bot the player to add
		 */
		void add(Player::Ptr bot);

		/**
		 * Removes a player from the team.
		 * \param index the index of the robot to remove
		 */
		void remove(unsigned int index);

		friend class World;
};

#endif

