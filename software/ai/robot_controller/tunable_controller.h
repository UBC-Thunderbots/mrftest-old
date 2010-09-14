#ifndef AI_ROBOT_CONTROLLER_TUNABLE_CONTROLLER_H
#define AI_ROBOT_CONTROLLER_TUNABLE_CONTROLLER_H

#include <string>
#include <vector>

namespace AI {
	namespace RC {
		/**
		 * Parameter tunable robot controller.
		 * All tunable robot controller should inherit this class.
		 * The parameter is a vector of doubles.
		 * This class is not thread-safe.
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
				 * Gets the name of each parameter.
				 * Unless defined by the subclass, this will always return a vector of question marks.
				 *
				 * \return the parameters' names.
				 */
				virtual const std::vector<std::string> get_params_name() const {
					size_t n = get_params().size();
					return std::vector<std::string>(n, "?");
				}

				/**
				 * Gets the minimum value of each parameter.
				 * Unless defined, returns 70% of the current values.
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
				 * Gets the maximum value of each parameter.
				 * Unless defined, returns 130% of the current value.
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
				 * Gets one instance of a tunable controller.
				 *
				 * \return the current TunableController, or a null pointer if none exists.
				 */
				static TunableController* get_instance();
		};
	}
}

#endif

