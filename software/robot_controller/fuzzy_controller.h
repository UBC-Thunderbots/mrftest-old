#ifndef ROBOT_CONTROLLER_FUZZY_CONTROLLER_H
#define ROBOT_CONTROLLER_FUZZY_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "world/player_impl.h"
#include "robot_controller/tunable_controller.h"

class fuzzy_controller : public robot_controller, public tunable_controller {
	public:

		void move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity);

		robot_controller_factory &get_factory() const;

		fuzzy_controller(player_impl::ptr player);
		
	 	void set_params(const std::vector<double>& params) {
			this->param = params;
		}

		const std::vector<double>& get_params() const {
			return param;
		}

		const std::vector<double>& get_params_min() const {
			return param_min;
		}

		const std::vector<double>& get_params_max() const {
			return param_max;
		}

	protected:
		player_impl::ptr robot;
		
		static std::vector<double> param_min;
		
		static std::vector<double> param_max;

		std::vector<double> param;
};

#endif

