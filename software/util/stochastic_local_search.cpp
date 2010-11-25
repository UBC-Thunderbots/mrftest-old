#include "util/stochastic_local_search.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <limits>

#warning the header file util/stochastic_local_search.h needs Doxygen comments on its functions.

StochasticLocalSearch::StochasticLocalSearch(const std::vector<double> &start, const std::vector<double> &min, const std::vector<double> &max) {
	bestCost = std::numeric_limits<double>::max();
	param_cur = start;
	param_best = start;
	param_min = min;
	param_max = max;
}

const std::vector<double> StochasticLocalSearch::get_params() const {
	return param_cur;
}

const std::vector<double> StochasticLocalSearch::get_best_params() const {
	return param_best;
}

void StochasticLocalSearch::set_cost(double cost) {
	if (cost < bestCost) {
		param_best = param_cur;
		bestCost = cost;
	} else if (cost > bestCost) {
		param_cur = param_best;
	}
}

void StochasticLocalSearch::revert() {
	param_cur = param_best;
}

void StochasticLocalSearch::hill_climb() {
	param_cur = param_best;
	int tries = 100;
	while (tries > 0) {
		--tries;
		int index = rand() % param_cur.size();
		if (param_min[index] == param_max[index]) {
			continue;
		}
		param_cur[index] = drand48() * (param_max[index] - param_min[index]) + param_min[index];
		break;
	}
}

/*
   void StochasticLocalSearch::random_restart() {
    int index = rand()%param_cur.size();
    param_cur[index] = drand48()*(param_max[index]-param_min[index]) + param_min[index];
    for (uint i = 0; i < param_min.size(); i++) param_cur[i] = drand48()*(param_max[i]-param_min[i]) + param_min[i];
   }
 */

