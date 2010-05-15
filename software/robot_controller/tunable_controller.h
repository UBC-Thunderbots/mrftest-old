#ifndef ROBOT_CONTROLLER_TUNABLE_CONTROLLER_H
#define ROBOT_CONTROLLER_TUNABLE_CONTROLLER_H

#include <string>
#include <vector>

/**
 * Parameter tunable robot controller.
 * All tunable robot controller should inherit this class.
 * Parameter is a vector of doubles.
 * Not thread-safe.
 */
class tunable_controller {
	public:
		tunable_controller();

		~tunable_controller();

		/**
		 * Changes the controller parameters.
		 */
		virtual void set_params(const std::vector<double>& params) = 0;

		/**
		 * Gets the array of parameters.
		 */
		virtual const std::vector<double>& get_params() const = 0;

		/**
		 * Gets the name of each parameter.
		 * Unless defined by the subclass,
		 * this will always return a vector of question marks.
		 */
		virtual const std::vector<std::string> get_params_name() const {
			size_t n = get_params().size();
			return std::vector<std::string>(n, "?");
		}

		/**
		 * Gets the minimum value of each parameter.
		 */
		virtual const std::vector<double>& get_params_min() const = 0;

		/**
		 * Gets the maximum value of each parameter.
		 */
		virtual const std::vector<double>& get_params_max() const = 0;

		/**
		 * Gets one instance of a tunable controller.
		 * Returns NULL if no such controller exist.
		 */
		static tunable_controller* get_instance();
};

#endif

