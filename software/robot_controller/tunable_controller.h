#ifndef ROBOT_CONTROLLER_TUNABLE_CONTROLLER_H
#define ROBOT_CONTROLLER_TUNABLE_CONTROLLER_H

#include <vector>

//
// parameter tunable robot controller
//
class tunable_controller {
	public:
		//
		// changes the controller parameters
		//
		virtual void set_params(const std::vector<double>& params) = 0;

		//
		// gets the array of parameters
		//
		virtual const std::vector<double>& get_params() const = 0;
};

#endif
