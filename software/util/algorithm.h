#ifndef UTIL_ALGORITHM_H
#define UTIL_ALGORITHM_H

#include <algorithm>
#include <functional>
#include <vector>

namespace {
	/**
	 * Checks if an element exists within an iterator range.
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
	template<typename Titer, typename Telem>
	bool exists(Titer begin, Titer end, const Telem &elem) {
		return std::find(begin, end, elem) != end;
	}

	/**
	 * Checks if an element exists within an iterator range that satisfies a predicate.
	 *
	 * \tparam Titer the type of the iterators.
	 *
	 * \tparam Tpred the type of predicate.
	 *
	 * \param[in] begin the first element in the range.
	 *
	 * \param[in] end the last-plus-one element in the range.
	 *
	 * \param[in] pred the predicate to apply.
	 *
	 * \return \c true if there exists an element \c e in the range [\p begin,
	 * \p end) such that \c pred(e) returns \c true, or \c false if not.
	 */
	template<typename Titer, typename Tpred>
	bool exists_if(Titer begin, Titer end, Tpred pred) {
		return std::find_if(begin, end, pred) != end;
	}

	/**
	 * Clamps a value to fall within a given range.
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
	template<typename T>
	T clamp(const T &value, const T &lower, const T &upper) {
		return std::min(std::max(value, lower), upper);
	}

	/**
	 * A comparator that orders small nonnegative integers based on the ordering of objects in a vector at corresponding positions.
	 *
	 * \tparam T the type of elements in the lookup table.
	 *
	 * \tparam Comp the type of the comparator between lookup table elements (defaults to \c std::less<T>).
	 */
	template<typename T, typename Comp = std::less<T> >
	class IndexComparator {
		public:
			/**
			 * Constructs a new IndexComparator.
			 *
			 * \param[in] tbl the lookup table to use, which is not copied and must remain valid until the IndexComparator has been destroyed.
			 *
			 * \param[in] comp the comparator to use (defaults to \c Comp()).
			 */
			IndexComparator(const std::vector<T> &tbl, Comp comp = Comp()) : tbl(tbl), comp(comp) {
			}

			/**
			 * Executes a comparison.
			 *
			 * \pre 0 ≤ \p x, \p y < \c tbl.size().
			 *
			 * \param[in] x the first number to compare.
			 *
			 * \param[in] y the second number to compare.
			 *
			 * \return \c true if \p x should precede \p y, that is, if <code>comp(tbl[x], tbl[y])</code>, or \c false if not.
			 */
			bool operator()(unsigned int x, unsigned int y) const {
				return comp(tbl[x], tbl[y]);
			}

		private:
			const std::vector<T> &tbl;
			const Comp comp;
	};
}

#endif

