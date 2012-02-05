#ifndef AI_COMMON_TEAM_H
#define AI_COMMON_TEAM_H

#include <cstddef>
#include <sigc++/signal.h>

namespace AI {
	namespace Common {
		/**
		 * \brief Exposes the basic API provided by all teams.
		 */
		class Team {
			public:
				/**
				 * \brief The possible values indicating which colour a team is.
				 */
				enum class Colour {
					YELLOW,
					BLUE,
				};

				/**
				 * \brief Returns the number of points scored by the team.
				 *
				 * \return the team's score.
				 */
				virtual unsigned int score() const = 0;

				/**
				 * \brief Returns the size of the team.
				 *
				 * \return the size of the team.
				 */
				virtual std::size_t size() const = 0;

				/**
				 * \brief Returns the signal that is fired after a team's membership has changed.
				 *
				 * \return the signal that is fired after a team's membership has changed.
				 */
				virtual sigc::signal<void> &signal_membership_changed() const = 0;
		};
	}
}

#endif

