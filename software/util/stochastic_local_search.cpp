#include "util/stochastic_local_search.h"
#include <limits>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>
using namespace std;

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
	steps = 0;
	improved = false;
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
		improved = true;
	} else {
		param_cur = param_best;
	}
}

void stochastic_local_search::hill_climb() {
	int index = rand()%param_cur.size();
	if (improved) {
		steps++;
	} else {
		param_cur = param_best;
	}
	double change = drand48()*(param_max[index]-param_min[index]) * exp(- steps * 10);
	// std::cout << param_min[index] << " " << change << " " << param_max[index] << std::endl;
	param_cur[index] = param_best[index] + change;
	param_cur[index] = max(param_min[index], param_cur[index]);
	param_cur[index] = min(param_max[index], param_cur[index]);
	//param_cur[index] = drand48()*(param_max[index]-param_min[index]) + param_min[index];
	improved = false;
}

void stochastic_local_search::random_restart() {
	steps = 0;
	int index = rand()%param_cur.size();
	param_cur[index] = drand48()*(param_max[index]-param_min[index]) + param_min[index];
	for (uint i = 0; i < param_min.size(); i++) param_cur[i] = drand48()*(param_max[i]-param_min[i]) + param_min[i];
	improved = false;
}

