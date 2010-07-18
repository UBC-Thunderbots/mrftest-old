#ifndef UTIL_HUNGARIAN_H
#define UTIL_HUNGARIAN_H

#include <vector>
#include <cassert>
#include <cstddef>

//
// Uses the Hungarian algorithm to perform a maximum-weight bipartite matching.
//
class Hungarian {
public:
	//
	// Constructs a new Hungarian. The "size" parameter indicates how many elements are in the left and right sets.
	//
	Hungarian(std::size_t size);

	//
	// Returns the dimension of the Hungarian matrix.
	//
	std::size_t size() const {
		return weights.size();
	}

	//
	// Gets or sets a pairwise weight of matching X-node "x" with Y-node "y".
	//
	double &weight(std::size_t x, std::size_t y) {
		assert(x < weights.size());
		assert(y < weights.size());
		return weights[x][y];
	}

	//
	// Gets a pairwise weight of matching X-node "x" with Y-node "y".
	//
	const double &weight(std::size_t x, std::size_t y) const {
		assert(x < weights.size());
		assert(y < weights.size());
		return weights[x][y];
	}

	//
	// Runs the algorithm.
	//
	void execute();

	//
	// Returns the index of the Y-node that should be matched with X-node "x".
	//
	std::size_t matchX(std::size_t x) const {
		assert(x < mx.size());
		return mx[x];
	}

	//
	// Returns the index of the X-node that should be matched with Y-node "y".
	//
	std::size_t matchY(std::size_t y) const {
		assert(y < my.size());
		return my[y];
	}

private:
	std::vector<std::vector<double> > weights;
	std::vector<std::size_t> mx, my;
};

#endif

