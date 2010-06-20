#include "util/stochastic_local_search.h"
#include <limits>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>
using namespace std;

stochastic_local_search::stochastic_local_search(const std::vector<double>& min, const std::vector<double>& max) {
	srand48(time(NULL));
	srand(time(NULL));
	bestCost = std::numeric_limits<double>::max();
	curCost = bestCost;
	param_cur = min;
	param_best = min;
	param_min = min;
	param_max = max;
	for (uint i = 0; i < min.size(); i++) param_cur[i] = drand48()*(max[i]-min[i]) + min[i];
	stale = 0;
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
		stale /= 2;
	} else if (cost > bestCost) {
		param_cur = param_best;
		stale++;
	}
}

void stochastic_local_search::set_initial(const std::vector<double>& initial) {
	param_cur = initial;
	param_best = initial;
	stale = 0;
}

void stochastic_local_search::revert() {
	param_cur = param_best;
}

void stochastic_local_search::hill_climb() {
	param_cur = param_best;
	int tries = 100;
	while(tries > 0) {
		--tries;
		int index = rand() % param_cur.size();
		if (param_min[index] == param_max[index]) continue;
		param_cur[index] = drand48()*(param_max[index]-param_min[index]) + param_min[index];
		break;
	}
}

void stochastic_local_search::random_restart() {
	int index = rand()%param_cur.size();
	param_cur[index] = drand48()*(param_max[index]-param_min[index]) + param_min[index];
	for (uint i = 0; i < param_min.size(); i++) param_cur[i] = drand48()*(param_max[i]-param_min[i]) + param_min[i];
	param_best = param_cur;
	stale = 0;
}

