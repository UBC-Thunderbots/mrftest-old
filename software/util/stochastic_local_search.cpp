using namespace std;
#include <limits>
#include <vector>
#include <stdlib.h>
#include <time.h>

class stochastic_local_search {
	private:
		double bestCost;
		double curCost;
		std::vector<double> param_cur;
		std::vector<double> param_best;
		std::vector<double> param_min;
		std::vector<double> param_max;

	public:
	
		/*
		Instructions on how to use this class:
		
		1) Initialize constructor with a min/max range for each parameter.
		2) Call "get_params" to get the current parameter settings.
		3) Evaluate the cost function (such as movement benchmark) and set the cost using "set_cost".
		4) Repeat steps 2-3 until you get bored.
		5) Call "get_best_params" to get the best parameter values.
		*/
	
		stochastic_local_search(const std::vector<double>& min, const std::vector<double>& max){
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
		
		const std::vector<double>& get_params() const {
			return param_cur;
		}
		
		const std::vector<double>& get_best_params() const {
			return param_best;
		}
		
		void set_cost(double cost) {
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
};

