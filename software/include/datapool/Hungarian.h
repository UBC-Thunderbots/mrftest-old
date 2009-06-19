#ifndef DATAPOOL_HUNGARIAN_H
#define DATAPOOL_HUNGARIAN_H

#include <vector>
#include <cassert>

//
// Uses the Hungarian algorithm to perform a maximum-weight bipartite matching.
//
class Hungarian {
public:
	//
	// Constructs a new Hungarian. The "size" parameter indicates how many elements are in the left and right sets.
	//
	Hungarian(unsigned int size);

	//
	// Gets or sets a pairwise weight of matching X-node "x" with Y-node "y".
	//
	double &weight(unsigned int x, unsigned int y) {
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
	unsigned int matchX(unsigned int x) {
		assert(x < mx.size());
		return mx[x];
	}

	//
	// Returns the index of the X-node that should be matched with Y-node "y".
	//
	unsigned int matchY(unsigned int y) {
		assert(y < my.size());
		return my[y];
	}

private:
	std::vector<std::vector<double> > weights;
	std::vector<unsigned int> mx, my;
};

#endif

