#ifndef UTIL_ALGORITHM_H
#define UTIL_ALGORITHM_H

#include <algorithm>
#include <functional>
#include <vector>

/**
 * \brief Checks if an element exists within an iterator range.
 *
 * \tparam Titer the type of the iterators.
 *
 * \tparam Telem the type of element in the range.
 *
 * \param[in] begin the first element in the range.
 *
 * \param[in] end the last-plus-one element in the range.
 *
 * \param[in] elem the element to look for.
 *
 * \return \c true if \p elem exists in the range [\p begin, \p end), or \c false if not.
 */
template<typename Titer, typename Telem> bool exists(Titer begin, Titer end, const Telem &elem);

/**
 * \brief Clamps a value to fall within a given range.
 *
 * \tparam T the type of value to clamp.
 *
 * \param[in] value the value to clamp.
 *
 * \param[in] lower the minimum legal value.
 *
 * \param[in] upper the maximum legal value.
 *
 * \return the nearest value to \p value that lies in the range [\p lower, \p upper].
 */
template<typename T> T clamp(const T &value, const T &lower, const T &upper);

/**
 * \brief Clamps a value to fall within a given range.
 *
 * \tparam T the type of value to clamp.
 *
 * \param[in] value the value to clamp.
 *
 * \param[in] limit the maximum legal magnitude.
 *
 * \return the nearest value to \p value that lies in the range ±\p limit.
 */
template<typename T> T clamp_symmetric(const T &value, const T &limit);

/**
 * \brief A comparator that orders small nonnegative integers based on the ordering of objects in a vector at corresponding positions.
 *
 * \tparam T the type of elements in the lookup table.
 *
 * \tparam Comp the type of the comparator between lookup table elements (defaults to \c std::less<T>).
 */
template<typename T, typename Comp = std::less<T>> class IndexComparator {
	public:
		/**
		 * \brief Constructs a new IndexComparator.
		 *
		 * \param[in] tbl the lookup table to use, which is not copied and must remain valid until the IndexComparator has been destroyed.
		 *
		 * \param[in] comp the comparator to use (defaults to \c Comp()).
		 */
		IndexComparator(const std::vector<T> &tbl, Comp comp = Comp());

		/**
		 * \brief Executes a comparison.
		 *
		 * \pre 0 ≤ \p x, \p y < \c tbl.size().
		 *
		 * \param[in] x the first number to compare.
		 *
		 * \param[in] y the second number to compare.
		 *
		 * \return \c true if \p x should precede \p y, that is, if <code>comp(tbl[x], tbl[y])</code>, or \c false if not.
		 */
		bool operator()(unsigned int x, unsigned int y) const;

	private:
		const std::vector<T> &tbl;
		Comp comp;
};



template<typename Titer, typename Telem> inline bool exists(Titer begin, Titer end, const Telem &elem) {
	return std::find(begin, end, elem) != end;
}

template<typename T> inline T clamp(const T &value, const T &lower, const T &upper) {
	return std::min(std::max(value, lower), upper);
}

template<typename T> inline T clamp_symmetric(const T &value, const T &limit) {
	return clamp(value, std::min(-limit, limit), std::max(-limit, limit));
}

template<typename T, typename Comp> inline IndexComparator<T, Comp>::IndexComparator(const std::vector<T> &tbl, Comp comp) : tbl(tbl), comp(comp) {
}

template<typename T, typename Comp> inline bool IndexComparator<T, Comp>::operator()(unsigned int x, unsigned int y) const {
	return comp(tbl[x], tbl[y]);
}

#endif

