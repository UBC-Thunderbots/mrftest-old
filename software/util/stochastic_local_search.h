#ifndef UTIL_STOCHASTIC_LOCAL_SEARCH_H
#define UTIL_STOCHASTIC_LOCAL_SEARCH_H

#include <vector>

/*
   Instructions on how to use this class:

   1) Initialize constructor with a min/max range for each parameter.
   2) Call "get_params" to get the current parameter settings.
   3) Evaluate the cost function (such as movement benchmark) and set the cost using "set_cost".
   4) Repeat steps 2-3 until you get bored.
   5) Call "get_best_params" to get the best parameter values.
 */

class stochastic_local_search {
	private:
		double bestCost;
		double curCost;
		int steps;
		bool improved;
		std::vector<double> param_cur;
		std::vector<double> param_best;
		std::vector<double> param_min;
		std::vector<double> param_max;
	public:
		stochastic_local_search(const std::vector<double>& min, const std::vector<double>& max);
		const std::vector<double>& get_params() const;
		const std::vector<double>& get_best_params() const;
		void set_cost(double cost);
		// change settings
		void random_restart();
		void hill_climb();
};


#endif

