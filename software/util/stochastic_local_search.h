#ifndef UTIL_STOCHASTIC_LOCAL_SEARCH_H
#define UTIL_STOCHASTIC_LOCAL_SEARCH_H

#include <vector>

/*
   Instructions on how to use this class:

   1) Initialize constructor with a min/max range for each parameter.
   2) Call "get_params" to get the current parameter settings.
   3) Evaluate the cost function (such as movement benchmark) and set the cost using "set_cost".
   4) Call hill_climb
   5) Goto step 2
   6) Call "get_best_params" to get the best parameter values.
   7) Call random_restart if u think solution is stuck on a local maximum/minima
 
   Some advice:
   - the LESSER the PARAMETERS, the faster the convergence
   - if the max of a parameter equals the min, then that parameter is ignored
   
 */

class stochastic_local_search {
	private:
		double bestCost;
		double curCost;
		int stale;
		std::vector<double> param_cur;
		std::vector<double> param_best;
		std::vector<double> param_min;
		std::vector<double> param_max;
	public:
		stochastic_local_search(const std::vector<double>& start, const std::vector<double>& min, const std::vector<double>& max);
		const std::vector<double> get_params() const;
		const std::vector<double> get_best_params() const;
		void set_initial(const std::vector<double>& initial);
		void set_cost(double cost);
		void random_restart();
		void hill_climb();
		void revert();
};

#endif

