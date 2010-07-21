#ifndef ROBOT_CONTROLLER_FUZZY_CONTROLLER_H
#define ROBOT_CONTROLLER_FUZZY_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "ai/world/player.h"
#include "robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "robot_controller/tunable_controller.h"

class FuzzyController : public RobotController, public TunableController {
	public:

		void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);

		void clear();

		RobotControllerFactory &get_factory() const;

		FuzzyController(Player::ptr player);
		
	 	void set_params(const std::vector<double>& params) {
			this->param = params;
		}

		const std::vector<double> get_params() const {
			return param;
		}

		const std::vector<double> get_params_default() const;

		const std::vector<double> get_params_min() const {
			return param_min;
		}

		const std::vector<double> get_params_max() const {
			return param_max;
		}

	protected:
		Player::ptr robot;
		
		static const std::vector<double> param_min;
		static const std::vector<double> param_max;
		static const std::vector<double> param_default;

		std::vector<double> param;
};

#endif

