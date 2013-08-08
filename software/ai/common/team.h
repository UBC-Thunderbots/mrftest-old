#ifndef AI_COMMON_TEAM_H
#define AI_COMMON_TEAM_H

#include "ai/backend/team.h"
#include <cstddef>
#include <sigc++/signal.h>

namespace AI {
	namespace Common {
		/**
		 * \brief The possible values indicating which colour a team is.
		 */
		enum class Colour {
			YELLOW,
			BLUE,
		};

		template<typename T, typename U> class Team;
		template<typename T, typename U> class TeamIterator;

		/**
		 * \brief Compares two iterators.
		 *
		 * \param[in] x the first iterator
		 *
		 * \param[in] y the second iterator
		 *
		 * \return \c true if the iterators point to the same element of the same team, or \c false if not
		 */
		template<typename T, typename U> bool operator==(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y);

		/**
		 * \brief Compares two iterators.
		 *
		 * \param[in] x the first iterator
		 *
		 * \param[in] y the second iterator
		 *
		 * \return \c false if the iterators point to the same element of the same team, or \c true if not
		 */
		template<typename T, typename U> bool operator!=(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y);

		/**
		 * \brief Compares two iterators.
		 *
		 * \param[in] x the first iterator
		 *
		 * \param[in] y the second iterator
		 *
		 * \return \c true if \p x is less than \p y, or \c false if not
		 */
		template<typename T, typename U> bool operator<(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y);

		/**
		 * \brief Compares two iterators.
		 *
		 * \param[in] x the first iterator
		 *
		 * \param[in] y the second iterator
		 *
		 * \return \c true if \p x is greater than \p y, or \c false if not
		 */
		template<typename T, typename U> bool operator>(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y);

		/**
		 * \brief Compares two iterators.
		 *
		 * \param[in] x the first iterator
		 *
		 * \param[in] y the second iterator
		 *
		 * \return \c true if \p x is less than or equal to \p y, or \c false if not
		 */
		template<typename T, typename U> bool operator<=(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y);

		/**
		 * \brief Compares two iterators.
		 *
		 * \param[in] x the first iterator
		 *
		 * \param[in] y the second iterator
		 *
		 * \return \c true if \p x is greater than or equal to \p y, or \c false if not
		 */
		template<typename T, typename U> bool operator>=(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y);

		/**
		 * \brief Advances an iterator.
		 *
		 * \param[in] i the iterator
		 *
		 * \param[in] n the distance to advance
		 *
		 * \return the advanced iterator
		 */
		template<typename T, typename U> TeamIterator<T, U> operator+(TeamIterator<T, U> i, typename TeamIterator<T, U>::difference_type n);

		/**
		 * \brief Advances an iterator.
		 *
		 * \param[in] n the distance to advance
		 *
		 * \param[in] i the iterator
		 *
		 * \return the advanced iterator
		 */
		template<typename T, typename U> TeamIterator<T, U> operator+(typename TeamIterator<T, U>::difference_type n, TeamIterator<T, U> i);

		/**
		 * \brief Reverses an iterator.
		 *
		 * \param[in] i the iterator
		 *
		 * \param[in] n the distance to reverse
		 *
		 * \return the reversed iterator
		 */
		template<typename T, typename U> TeamIterator<T, U> operator-(TeamIterator<T, U> i, typename TeamIterator<T, U>::difference_type n);

		/**
		 * \brief Computes the difference between two iterators.
		 *
		 * \param[in] x the first iterator
		 *
		 * \param[in] y the second iterator
		 *
		 * \return the distance between iterators
		 */
		template<typename T, typename U> typename TeamIterator<T, U>::difference_type operator-(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y);

		/**
		 * \brief Exchanges two iterators.
		 *
		 * \param[in] x the first iterator
		 *
		 * \param[in] y the second iterator
		 */
		template<typename T, typename U> void swap(TeamIterator<T, U> &x, TeamIterator<T, U> &y);

		template<typename T, typename U> class TeamIterator : public std::iterator<std::random_access_iterator_tag, T, ptrdiff_t, T*, T> {
			public:
				/**
				 * \brief Constructs a new iterator.
				 *
				 * \param[in] team the team to iterate over
				 *
				 * \param[in] pos the index of the robot to point to
				 */
				TeamIterator(const Team<T, U> *team, typename TeamIterator<T, U>::difference_type pos);

				/**
				 * \brief Returns the current object.
				 *
				 * \return the current object
				 */
				T operator*() const;

				/**
				 * \brief Returns a pointer to the current object.
				 *
				 * \return a pointer to the current object
				 */
				T *operator->() const;

				/**
				 * \brief Returns an object at another location in the team.
				 *
				 * \param[in] n the offset
				 *
				 * \return the object
				 */
				T operator[](typename TeamIterator<T, U>::difference_type n) const;

				/**
				 * \brief Advances the iterator.
				 *
				 * \return the iterator
				 */
				TeamIterator &operator++();

				/**
				 * \brief Advances the iterator.
				 *
				 * \return a copy of the original iterator
				 */
				TeamIterator operator++(int);

				/**
				 * \brief Advances the iterator by multiple positions.
				 *
				 * \param[in] n the distance to advance
				 *
				 * \return the iterator
				 */
				TeamIterator &operator+=(typename TeamIterator<T, U>::difference_type n);

				/**
				 * \brief Reverses the iterator.
				 *
				 * \return the iterator
				 */
				TeamIterator &operator--();

				/**
				 * \brief Reverses the iterator.
				 *
				 * \return a copy of the original iterator
				 */
				TeamIterator operator--(int);

				/**
				 * \brief Reverses the iterator by multiple positions.
				 *
				 * \param[in] n the distance to reverse
				 *
				 * \return the iterator
				 */
				TeamIterator &operator-=(typename TeamIterator<T, U>::difference_type n);

			private:
				friend bool operator==<T, U>(const TeamIterator &, const TeamIterator &);
				friend typename TeamIterator<T, U>::difference_type operator-<T, U>(const TeamIterator &, const TeamIterator &);
				friend void swap<T, U>(TeamIterator &, TeamIterator &);

				const Team<T, U> *team;
				typename TeamIterator<T, U>::difference_type pos;
				T obj;

				void update_obj();
		};

		/**
		 * \brief Exposes the basic API provided by all teams.
		 *
		 * \tparam T the type of robot on the team as exposed to the frontend
		 *
		 * \tparam U the type of robot on the team as implemented in the backend
		 */
		template<typename T, typename U> class Team {
			public:
				/**
				 * \brief The type of an iterator across the members of a team.
				 */
				typedef TeamIterator<T, U> iterator;

				/**
				 * \brief Constructs a new Team.
				 *
				 * \param[in] impl the backend implementation
				 */
				Team(const AI::BE::Team<U> &impl);

				/**
				 * \brief Constructs a copy of a Team.
				 *
				 * \param[in] copyref the object to copy
				 */
				Team(const Team<T, U> &copyref);

				/**
				 * \brief Returns the pattern number of the team’s goalie.
				 *
				 * \return the team’s goalie
				 */
				unsigned int goalie() const;

				/**
				 * \brief Returns the number of points scored by the team.
				 *
				 * \return the team’s score
				 */
				unsigned int score() const;

				/**
				 * \brief Returns an iterator pointing to the first robot on the team.
				 *
				 * \return the start iterator
				 */
				iterator begin() const;

				/**
				 * \brief Returns an iterator pointing past the end of the team.
				 *
				 * \return the end iterator
				 */
				iterator end() const;

				/**
				 * \brief Returns the size of the team.
				 *
				 * \return the size of the team
				 */
				std::size_t size() const;

				/**
				 * \brief Returns a robot from the team.
				 *
				 * \param[in] i the index of the robot
				 *
				 * \return the robot
				 */
				T operator[](std::size_t i) const;

				/**
				 * \brief Returns the signal that is fired after a team’s membership has changed.
				 *
				 * \return the signal that is fired after a team’s membership has changed
				 */
				sigc::signal<void> &signal_membership_changed() const;

			protected:
				const AI::BE::Team<U> &impl;
		};
	}
}



template<typename T, typename U> inline bool AI::Common::operator==(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y) {
	return x.team == y.team && x.pos == y.pos;
}

template<typename T, typename U> inline bool AI::Common::operator!=(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y) {
	return !(x == y);
}

template<typename T, typename U> inline bool AI::Common::operator<(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y) {
	return x.pos < y.pos;
}

template<typename T, typename U> inline bool AI::Common::operator>(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y) {
	return x.pos > y.pos;
}

template<typename T, typename U> inline bool AI::Common::operator<=(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y) {
	return x.pos <= y.pos;
}

template<typename T, typename U> inline bool AI::Common::operator>=(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y) {
	return x.pos >= y.pos;
}

template<typename T, typename U> inline AI::Common::TeamIterator<T, U> AI::Common::operator+(TeamIterator<T, U> i, typename TeamIterator<T, U>::difference_type n) {
	i += n;
	return i;
}

template<typename T, typename U> inline AI::Common::TeamIterator<T, U> AI::Common::operator+(typename TeamIterator<T, U>::difference_type n, TeamIterator<T, U> i) {
	return i + n;
}

template<typename T, typename U> inline AI::Common::TeamIterator<T, U> AI::Common::operator-(TeamIterator<T, U> i, typename TeamIterator<T, U>::difference_type n) {
	i -= n;
	return i;
}

template<typename T, typename U> inline typename AI::Common::TeamIterator<T, U>::difference_type AI::Common::operator-(const TeamIterator<T, U> &x, const TeamIterator<T, U> &y) {
	return y.pos - x.pos;
}

template<typename T, typename U> inline void AI::Common::swap(TeamIterator<T, U> &x, TeamIterator<T, U> &y) {
	using std::swap;
	swap(x.team, y.team);
	swap(x.pos, y.pos);
	swap(x.obj, y.obj);
}

template<typename T, typename U> inline AI::Common::TeamIterator<T, U>::TeamIterator(const Team<T, U> *team, typename TeamIterator<T, U>::difference_type pos) : team(team), pos(pos) {
	update_obj();
}

template<typename T, typename U> inline T AI::Common::TeamIterator<T, U>::operator*() const {
	return obj;
}

template<typename T, typename U> inline T *AI::Common::TeamIterator<T, U>::operator->() const {
	return &obj;
}

template<typename T, typename U> inline T AI::Common::TeamIterator<T, U>::operator[](typename TeamIterator<T, U>::difference_type n) const {
	return T((*team)[pos + n]);
}

template<typename T, typename U> inline AI::Common::TeamIterator<T, U> &AI::Common::TeamIterator<T, U>::operator++() {
	return *this += 1;
}

template<typename T, typename U> inline AI::Common::TeamIterator<T, U> AI::Common::TeamIterator<T, U>::operator++(int) {
	TeamIterator<T, U> tmp(*this);
	++*this;
	return tmp;
}

template<typename T, typename U> inline AI::Common::TeamIterator<T, U> &AI::Common::TeamIterator<T, U>::operator+=(typename TeamIterator<T, U>::difference_type n) {
	pos += n;
	update_obj();
	return *this;
}

template<typename T, typename U> inline AI::Common::TeamIterator<T, U> &AI::Common::TeamIterator<T, U>::operator--() {
	return *this -= 1;
}

template<typename T, typename U> inline AI::Common::TeamIterator<T, U> AI::Common::TeamIterator<T, U>::operator--(int) {
	TeamIterator<T, U> tmp(*this);
	--*this;
	return tmp;
}

template<typename T, typename U> inline AI::Common::TeamIterator<T, U> &AI::Common::TeamIterator<T, U>::operator-=(typename TeamIterator<T, U>::difference_type n) {
	pos -= n;
	update_obj();
	return *this;
}

template<typename T, typename U> inline void AI::Common::TeamIterator<T, U>::update_obj() {
	if (pos < static_cast<typename TeamIterator<T, U>::difference_type>(team->size())) {
		obj = (*team)[pos];
	} else {
		obj = T();
	}
}



template<typename T, typename U> inline AI::Common::Team<T, U>::Team(const AI::BE::Team<U> &impl) : impl(impl) {
}

template<typename T, typename U> inline AI::Common::Team<T, U>::Team(const Team<T, U> &) = default;

template<typename T, typename U> inline unsigned int AI::Common::Team<T, U>::goalie() const {
	return impl.goalie;
}

template<typename T, typename U> inline unsigned int AI::Common::Team<T, U>::score() const {
	return impl.score;
}

template<typename T, typename U> inline typename AI::Common::Team<T, U>::iterator AI::Common::Team<T, U>::begin() const {
	return iterator(this, 0);
}

template<typename T, typename U> inline typename AI::Common::Team<T, U>::iterator AI::Common::Team<T, U>::end() const {
	return iterator(this, size());
}

template<typename T, typename U> inline std::size_t AI::Common::Team<T, U>::size() const {
	return impl.size();
}

template<typename T, typename U> inline T AI::Common::Team<T, U>::operator[](std::size_t i) const {
	return T(impl.get(i));
}

template<typename T, typename U> inline sigc::signal<void> &AI::Common::Team<T, U>::signal_membership_changed() const {
	return impl.signal_membership_changed();
}

#endif

