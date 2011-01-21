#ifndef AI_ROBOT_CONTROLLER_PID_CONTROLLER_H
#define AI_ROBOT_CONTROLLER_PID_CONTROLLER_H

#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include <glibmm.h>
#include <vector>

namespace AI {
	namespace RC {
		class TunablePIDController : public OldRobotController, public TunableController {
			public:
				void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);

				void clear();

				RobotControllerFactory &get_factory() const;

				TunablePIDController(AI::RC::W::World &world, AI::RC::W::Player::Ptr plr);

				void set_params(const std::vector<double> &params) {
					this->param = params;
				}

				const std::vector<std::string> get_params_name() const;

				const std::vector<double> get_params() const {
					return param;
				}

				const std::vector<double> get_params_default() const {
					return param_default;
				}

				const std::vector<double> get_params_min() const {
					return param_min;
				}

				const std::vector<double> get_params_max() const {
					return param_max;
				}

			protected:
				static const std::vector<double> param_min;
				static const std::vector<double> param_max;
				static const std::vector<double> param_default;

				bool initialized;

				std::vector<double> param;

				// errors in x, y, d
				std::vector<Point> error_pos;
				std::vector<double> error_ori;

				Point prev_new_pos;
				double prev_new_ori;
		};
	}
}

#endif

