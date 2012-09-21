#ifndef AI_COMMON_TEAM_H
#define AI_COMMON_TEAM_H

#include "ai/backend/team.h"
#include <cstddef>
#include <sigc++/signal.h>

namespace AI {
	namespace Common {
		/**
		 * \brief The possible values indicating which colour a team is
		 */
		enum class Colour {
			YELLOW,
			BLUE,
		};

		/**
		 * \brief Exposes the basic API provided by all teams
		 *
		 * \tparam T the type of robot on the team as exposed to the frontend
		 *
		 * \tparam U the type of robot on the team as implemented in the backend
		 */
		template<typename T, typename U> class Team {
			public:
				/**
				 * \brief Constructs a new Team
				 *
				 * \param[in] impl the backend implementation
				 */
				Team(const AI::BE::Team<U> &impl);

				/**
				 * \brief Constructs a copy of a Team
				 *
				 * \param[in] copyref the object to copy
				 */
				Team(const Team<T, U> &copyref);

				/**
				 * \brief Returns the number of points scored by the team
				 *
				 * \return the team’s score
				 */
				unsigned int score() const;

				/**
				 * \brief Returns the size of the team
				 *
				 * \return the size of the team
				 */
				std::size_t size() const;

				/**
				 * \brief Returns a robot from the team
				 *
				 * \param[in] i the index of the robot
				 *
				 * \return the robot
				 */
				T get(std::size_t i) const;

				/**
				 * \brief Returns the signal that is fired after a team’s membership has changed
				 *
				 * \return the signal that is fired after a team’s membership has changed
				 */
				sigc::signal<void> &signal_membership_changed() const;

			protected:
				const AI::BE::Team<U> &impl;
		};
	}
}



template<typename T, typename U> inline AI::Common::Team<T, U>::Team(const AI::BE::Team<U> &impl) : impl(impl) {
}

template<typename T, typename U> inline AI::Common::Team<T, U>::Team(const Team<T, U> &) = default;

template<typename T, typename U> inline unsigned int AI::Common::Team<T, U>::score() const {
	return impl.score();
}

template<typename T, typename U> inline std::size_t AI::Common::Team<T, U>::size() const {
	return impl.size();
}

template<typename T, typename U> inline T AI::Common::Team<T, U>::get(std::size_t i) const {
	return T(impl.get(i));
}

template<typename T, typename U> inline sigc::signal<void> &AI::Common::Team<T, U>::signal_membership_changed() const {
	return impl.signal_membership_changed();
}

#endif

