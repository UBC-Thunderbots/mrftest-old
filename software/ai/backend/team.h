#ifndef AI_BACKEND_TEAM_H
#define AI_BACKEND_TEAM_H

#include "util/noncopyable.h"
#include "util/property.h"
#include <sigc++/signal.h>

namespace AI {
	namespace BE {
		/**
		 * \brief A team.
		 *
		 * \tparam T the type of robot in the team
		 */
		template<typename T> class Team : public NonCopyable {
			public:
				/**
				 * \brief The team’s score.
				 */
				Property<unsigned int> score;

				/**
				 * \brief The team’s goalie.
				 */
				Property<unsigned int> goalie;

				/**
				 * \brief Constructs a new Team.
				 */
				explicit Team();

				/**
				 * \brief Returns the size of the team.
				 *
				 * \return the size of the team
				 */
				virtual std::size_t size() const = 0;

				/**
				 * \brief Returns a player from the team.
				 *
				 * \param[in] i the index of the player
				 *
				 * \return the player
				 */
				virtual typename T::Ptr get(std::size_t i) const = 0;

				/**
				 * \brief Returns the signal that is fired after a team’s membership has changed.
				 *
				 * \return the signal that is fired after a team’s membership has changed
				 */
				sigc::signal<void> &signal_membership_changed() const;

			private:
				mutable sigc::signal<void> signal_membership_changed_;
		};
	}
}



template<typename T> inline AI::BE::Team<T>::Team() : score(0U), goalie(0U) {
}

template<typename T> inline sigc::signal<void> &AI::BE::Team<T>::signal_membership_changed() const {
	return signal_membership_changed_;
}

#endif

