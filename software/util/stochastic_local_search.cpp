using namespace std;
#include "util/stochastic_local_search.h"
#include <limits>
#include <stdlib.h>
#include <time.h>

stochastic_local_search::stochastic_local_search(const std::vector<double>& min, const std::vector<double>& max){
	srand48(time(NULL));
	srand(time(NULL));
	bestCost = numeric_limits<double>::max();
	curCost = bestCost;
	param_cur = min;
	param_best = min;
	param_min = min;
	param_max = max;
	for (uint i = 0; i < min.size(); i++) param_cur[i] = drand48()*(max[i]-min[i]) + min[i];
}

const std::vector<double>& stochastic_local_search::get_params() const {
	return param_cur;
}

const std::vector<double>& stochastic_local_search::get_best_params() const {
	return param_best;
}

void stochastic_local_search::set_cost(double cost) {
	curCost = cost;
	if (cost < bestCost) {
		param_best = param_cur;
		bestCost = cost;
	} else {
		param_cur = param_best;
	}
	int index = rand()%param_cur.size();
	param_cur[index] = drand48()*(param_max[index]-param_min[index]) + param_min[index];
}

