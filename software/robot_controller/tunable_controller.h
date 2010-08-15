#ifndef ROBOT_CONTROLLER_TUNABLE_CONTROLLER_H
#define ROBOT_CONTROLLER_TUNABLE_CONTROLLER_H

#include <string>
#include <vector>

/**
 * Parameter tunable robot controller. All tunable robot controller should
 * inherit this class. Parameter is a vector of doubles. NOT thread-safe.
 */
class TunableController {
	public:
		/**
		 * Constructs a new TunableController.
		 */
		TunableController();

		/**
		 * Destroys a TunableController.
		 */
		~TunableController();

		/**
		 * Changes the controller parameters.
		 *
		 * \param[in] params the new parameter values.
		 */
		virtual void set_params(const std::vector<double>& params) = 0;

		/**
		 * Gets the array of parameters.
		 *
		 * \return the current parameter values.
		 */
		virtual const std::vector<double> get_params() const = 0;

		/**
		 * Gets the default array of parameters.
		 *
		 * \return the default values of the parameters.
		 */
		virtual const std::vector<double> get_params_default() const = 0;

		/**
		 * Gets the name of each parameter. Unless defined by the subclass, this
		 * will always return a vector of question marks.
		 *
		 * \return the parameters' names.
		 */
		virtual const std::vector<std::string> get_params_name() const {
			size_t n = get_params().size();
			return std::vector<std::string>(n, "?");
		}

		/**
		 * Gets the minimum value of each parameter. Unless defined, returns the
		 * default value.
		 *
		 * \return the minimum values.
		 */
		virtual const std::vector<double> get_params_min() const {
			std::vector<double> ret = get_params();
			for (size_t i = 0; i < ret.size(); ++i)
				ret[i] *= 0.7;
			return ret;
		}

		/**
		 * Gets the maximum value of each parameter. Unless defined, returns the
		 * default value.
		 *
		 * \return the maximum values.
		 */
		virtual const std::vector<double> get_params_max() const {
			std::vector<double> ret = get_params();
			for (size_t i = 0; i < ret.size(); ++i)
				ret[i] *= 1.3;
			return ret;
		}

		/**
		 * Gets one instance of a tunable controller. Returns NULL if no such
		 * controller exist.
		 *
		 * \return the current TunableController.
		 */
		static TunableController* get_instance();
};

#endif

