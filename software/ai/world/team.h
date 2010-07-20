#ifndef AI_WORLD_TEAM_H
#define AI_WORLD_TEAM_H

#include "ai/world/player.h"
#include "ai/world/robot.h"
#include "util/memory.h"
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
		 * Fired when a robot is added to the team.
		 */
		mutable sigc::signal<void, unsigned int, RefPtr<Robot> > signal_robot_added;

		/**
		 * Fired when a robot is removed from the team.
		 */
		mutable sigc::signal<void, unsigned int, RefPtr<Robot> > signal_robot_removed;

		/**
		 * \return The number of robots on the team.
		 */
		virtual std::size_t size() const = 0;

		/**
		 * \param index the index of the robot to fetch
		 * \return The robot
		 */
		virtual RefPtr<Robot> get_robot(unsigned int index) const = 0;

		/**
		 * \return A vector of robots.
		 */
		virtual std::vector<RefPtr<Robot> > get_robots() const = 0;

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
		 * \return The number of robots on the team
		 */
		std::size_t size() const {
			return members.size();
		}

		/**
		 * \param index the index of the robot to fetch
		 * \return The robot
		 */
		RefPtr<Robot> get_robot(unsigned int index) const {
			assert(index < size());
			return members[index];
		}

		/**
		 * \param index the index of the robot to fetch
		 * \return The robot
		 */
		RefPtr<Robot> operator[](unsigned int index) const {
			assert(index < size());
			return members[index];
		}

		/**
		 * \return A vector of robots.
		 */
		std::vector<RefPtr<Robot> > get_robots() const {
			return members;
		}

	private:
		std::vector<RefPtr<Robot> > members;

		/**
		 * Creates a new EnemyTeam.
		 */
		static RefPtr<EnemyTeam> create();

		/**
		 * Adds a robot to the team.
		 * \param bot the robot to add
		 */
		void add(RefPtr<Robot> bot);

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
		 * Fired when a player is added to the team.
		 */
		mutable sigc::signal<void, unsigned int, RefPtr<Player> > signal_player_added;

		/**
		 * Fired when a player is removed from the team.
		 */
		mutable sigc::signal<void, unsigned int, RefPtr<Player> > signal_player_removed;

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
		RefPtr<Robot> get_robot(unsigned int index) const {
			return get_player(index);
		}

		/**
		 * \param index the index of the player to fetch
		 * \return The player
		 */
		RefPtr<Player> get_player(unsigned int index) const {
			assert(index < size());
			return members[index];
		}

		/**
		 * \param index the index of the player to fetch
		 * \return The player
		 */
		RefPtr<Player> operator[](unsigned int index) const {
			assert(index < size());
			return members[index];
		}

		/**
		 * \return A vector of players.
		 */
		const std::vector<RefPtr<Player> > &get_players() const {
			return members;
		}

		/**
		 * \return A vector of robots.
		 */
		std::vector<RefPtr<Robot> > get_robots() const {
			return std::vector<RefPtr<Robot> >(members.begin(), members.end());
		}

	private:
		std::vector<RefPtr<Player> > members;

		/**
		 * Creates a new FriendlyTeam.
		 */
		static RefPtr<FriendlyTeam> create();

		/**
		 * Adds a player to the team.
		 * \param bot the player to add
		 */
		void add(RefPtr<Player> bot);

		/**
		 * Removes a player from the team.
		 * \param index the index of the robot to remove
		 */
		void remove(unsigned int index);

		friend class World;
};

#endif

