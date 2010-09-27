#ifndef UTIL_STOCHASTIC_LOCAL_SEARCH_H
#define UTIL_STOCHASTIC_LOCAL_SEARCH_H

#include <vector>

/**
 * Instructions on how to use this class:
 *
 * <ol>
 * <li>Initialize constructor with a min/max range for each parameter.</li>
 * <li>Call get_params() const to get the current parameter settings.</li>
 * <li>Evaluate the cost function (such as movement benchmark) and set the cost using set_cost(double).</li>
 * <li>Call hill_climb().</li>
 * <li>Goto step 2.</li>
 * <li>Call get_best_params() const to get the best parameter values.</li>
 * <li>Call random_restart() if you think solution is stuck on a local maximum/minimum.</li>
 * </ol>
 *
 * Some advice:
 * \li the LESSER the PARAMETERS, the faster the convergence
 * \li if the max of a parameter equals the min, then that parameter is ignored
 */
class StochasticLocalSearch {
	private:
		double bestCost;
		std::vector<double> param_cur;
		std::vector<double> param_best;
		std::vector<double> param_min;
		std::vector<double> param_max;

	public:
		StochasticLocalSearch(const std::vector<double> &start, const std::vector<double> &min, const std::vector<double> &max);
		const std::vector<double> get_params() const;
		const std::vector<double> get_best_params() const;
		void set_cost(double cost);
		// void random_restart();
		void hill_climb();
		void revert();
};

#endif

