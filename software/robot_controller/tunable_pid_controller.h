#ifndef ROBOT_CONTROLLER_PID_CONTROLLER_H
#define ROBOT_CONTROLLER_PID_CONTROLLER_H

#include "ai/world/player.h"
#include "robot_controller/robot_controller.h"
#include "robot_controller/tunable_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"

#include <vector>
#include <glibmm.h>

class tunable_pid_controller : public robot_controller, public tunable_controller {
	public:

		void move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity);

		void clear();

		robot_controller_factory &get_factory() const;

		tunable_pid_controller(player::ptr plr);

	 	void set_params(const std::vector<double>& params) {
			this->param = params;
		}

		const std::vector<std::string> get_params_name() const;

		const std::vector<double> get_params() const {
			return param;
		}

		const std::vector<double> get_params_min() const {
			return param_min;
		}

		const std::vector<double> get_params_max() const {
			return param_max;
		}

	private:
		player::ptr plr;

	protected:

		static const std::vector<double> param_min;
		static const std::vector<double> param_max;
		static const std::vector<double> param_default;

		bool initialized;

		std::vector<double> param;

		// errors in x, y, d
		std::vector<point> error_pos;
		std::vector<double> error_ori;
		
		point prev_new_pos;
		double prev_new_ori;
};

#endif

