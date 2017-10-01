#ifndef UTIL_STOCHASTIC_LOCAL_SEARCH_H
#define UTIL_STOCHASTIC_LOCAL_SEARCH_H

#include <vector>

/**
 * Instructions on how to use this class:
 *
 * <ol>
 * <li>Initialize constructor with a min/max range for each parameter.</li>
 * <li>Call get_params() const to get the current parameter settings.</li>
 * <li>Evaluate the cost function (such as movement benchmark) and set the cost
 * using set_cost(double).</li>
 * <li>Call hill_climb().</li>
 * <li>Goto step 2.</li>
 * <li>Call get_best_params() const to get the best parameter values.</li>
 * <li>Call random_restart() if you think solution is stuck on a local
 * maximum/minimum.</li>
 * </ol>
 *
 * Some advice:
 * \li the LESSER the PARAMETERS, the faster the convergence
 * \li if the max of a parameter equals the min, then that parameter is ignored
 */
class StochasticLocalSearch final
{
   private:
    /**
     * The best (lowest) cost found so far by the search.
     */
    double bestCost;

    /**
     * The current parameters the search is using.
     */
    std::vector<double> param_cur;

    /**
     * The best parameters found so far.
     */
    std::vector<double> param_best;

    /**
     * The minimum values that each parameter can have.
     */
    std::vector<double> param_min;

    /**
     * The maximum values that each parameter can have.
     */
    std::vector<double> param_max;

   public:
    /**
     * Creates a stochastic local search with some starting parameter values.
     *
     * \param[in] start The list of starting values for the search.
     *
     * \param[in] min The list of minimum values that each parameter can have.
     *
     * \param[in] max The list of maximum values that each parameter can have.
     */
    explicit StochasticLocalSearch(
        const std::vector<double> &start, const std::vector<double> &min,
        const std::vector<double> &max);

    /**
     * Get the current list of parameters being used.
     *
     * \return List of current parameters.
     */
    const std::vector<double> get_params() const;

    /*
     * Get the best list of parameters found so far.
     *
     * \return List of best parameters.
     */
    const std::vector<double> get_best_params() const;

    /**
     * If the cost is better than the current best cost then set the current
     * parameters to be the best parameters and
     * the cost to be the best cost.  Otherwise the current parameters are set
     * to the best parameters.
     *
     * \param[in] cost The cost to check.
     */
    void set_cost(double cost);
    // void random_restart();

    /**
     * Pick a parameter at random and then set it's value to a random value
     * within it's min and max values.
     */
    void hill_climb();

    /**
     * Set the current parameters back to the best parameters.
     */
    void revert();
};

#endif
