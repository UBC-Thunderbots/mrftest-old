#ifndef AI_BACKEND_TEAM_H
#define AI_BACKEND_TEAM_H

#include "util/noncopyable.h"
#include <sigc++/signal.h>

namespace AI {
	namespace BE {
		/**
		 * \brief A team
		 *
		 * \tparam T the type of robot in the team
		 */
		template<typename T> class Team : public NonCopyable {
			public:
				/**
				 * \brief Returns the size of the team
				 *
				 * \return the size of the team
				 */
				virtual std::size_t size() const = 0;

				/**
				 * \brief Returns a player from the team
				 *
				 * \param[in] i the index of the player
				 *
				 * \return the player
				 */
				virtual typename T::Ptr get(std::size_t i) const = 0;

				/**
				 * \brief Returns the signal that is fired after a team’s membership has changed
				 *
				 * \return the signal that is fired after a team’s membership has changed
				 */
				sigc::signal<void> &signal_membership_changed() const;

				/**
				 * \brief Returns the number of points scored by the team.
				 *
				 * \return the team’s score
				 */
				virtual unsigned int score() const = 0;

			private:
				mutable sigc::signal<void> signal_membership_changed_;
		};
	}
}



template<typename T> inline sigc::signal<void> &AI::BE::Team<T>::signal_membership_changed() const {
	return signal_membership_changed_;
}

#endif

