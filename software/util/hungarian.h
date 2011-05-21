#ifndef UTIL_HUNGARIAN_H
#define UTIL_HUNGARIAN_H

#include <cassert>
#include <cstddef>
#include <vector>

/**
 * Uses the Hungarian algorithm to perform a maximum-weight bipartite matching.
 *
 * In other terms, given a square matrix of <var>N</var> rows and columns,
 * and given a number in each cell, selects <var>N</var> of those cells such that:
 * <ul>
 * <li>No two cells in the same row or column are chosen, and</li>
 * <li>The sum of values in the cells is maximized</li>
 * </ul>
 *
 * Usage:
 * <ol>
 * <li>You create a new Hungarian object with some size.
 * The object contains two node-sets, named <var>X</var> and <var>Y</var>, each of the given size.</li>
 * <li>You define the weight of matching each node in the <var>X</var> set with every node in the <var>Y</var> set,
 * using the weight(std::size_t, std::size_t) function.</li>
 * <li>You call execute(). This computes a matching where every node in <var>X</var> is matched with exactly one element in <var>Y</var>
 * (and consequently vice-versa) such that no other matching exists that has a higher total weight,
 * where the total weight of a matching is defined as the sum of the weights specified for all the matched pairs.</li>
 * <li>You read off the matching by either calling matchX(std::size_t) const to determine which element in <var>Y</var> matches with each element in <var>X</var>,
 * or by calling matchY(std::size_t) const to determine which element in <var>X</var> matches with each element in <var>Y</var>.</li>
 * </ol>
 */
class Hungarian {
	public:
		/**
		 * Constructs a new Hungarian.
		 *
		 * \param[in] size the number of elements in the left and right sets.
		 */
		Hungarian(std::size_t size);

		/**
		 * Returns the dimension of the Hungarian matrix.
		 */
		std::size_t size() const {
			return weights.size();
		}

		/**
		 * Gets or sets a pairwise weight.
		 *
		 * \param[in] x the index of a node in the <var>X</var> set.
		 *
		 * \param[in] y the index of a node in the <var>Y</var> set.
		 *
		 * \return the weight associated with matching these nodes.
		 */
		double &weight(std::size_t x, std::size_t y) {
			assert(x < weights.size());
			assert(y < weights.size());
			return weights[x][y];
		}

		/**
		 * Gets a pairwise weight.
		 *
		 * \param[in] x the index of a node in the <var>X</var> set.
		 *
		 * \param[in] y the index of a node in the <var>Y</var> set.
		 *
		 * \return the weight associated with matching these nodes.
		 */
		const double &weight(std::size_t x, std::size_t y) const {
			assert(x < weights.size());
			assert(y < weights.size());
			return weights[x][y];
		}

		/**
		 * Runs the algorithm.
		 */
		void execute();

		/**
		 * Looks up an element of the matching.
		 *
		 * \param[in] x the index of a node in the <var>X</var> set.
		 *
		 * \return the index of the node in the <var>Y</var> set that is matched with node \p x.
		 */
		std::size_t matchX(std::size_t x) const {
			assert(x < mx.size());
			return mx[x];
		}

		/**
		 * Looks up an element of the matching.
		 *
		 * \param[in] y the index of a node in the <var>Y</var> set.
		 *
		 * \return the index of the node in the <var>X</var> set that is matched with node \p y.
		 */
		std::size_t matchY(std::size_t y) const {
			assert(y < my.size());
			return my[y];
		}

	private:
		std::vector<std::vector<double> > weights;
		std::vector<std::size_t> mx, my;
};

#endif

