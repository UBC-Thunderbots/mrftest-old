#ifndef AI_COMMON_TEAM_H
#define AI_COMMON_TEAM_H

#include <cstddef>
#include <sigc++/sigc++.h>

namespace AI {
	namespace Common {
		/**
		 * Exposes the basic API provided by all teams.
		 */
		class Team {
			public:
				/**
				 * The possible values indicating which colour a team is.
				 */
				enum Colour {
					YELLOW,
					BLUE,
				};

				/**
				 * Returns the number of points scored by the team.
				 *
				 * \return the team's score.
				 */
				virtual unsigned int score() const = 0;

				/**
				 * Returns the size of the team.
				 *
				 * \return the size of the team.
				 */
				virtual std::size_t size() const = 0;

				/**
				 * Returns the signal that is fired when a robot is added to the team.
				 *
				 * \return the signal that is fired when a robot is added to the team.
				 */
				virtual sigc::signal<void, std::size_t> &signal_robot_added() const = 0;

				/**
				 * Returns the signal that is fired when a robot is about to be removed from the team.
				 *
				 * \return the signal that is fired when a robot is about to be removed from the team.
				 */
				virtual sigc::signal<void, std::size_t> &signal_robot_removing() const = 0;

				/**
				 * Returns the signal that is fired when a robot has been removed from the team.
				 *
				 * \return the signal that is fired when a robot has been removed from the team.
				 */
				virtual sigc::signal<void> &signal_robot_removed() const = 0;
		};
	}
}

#endif

