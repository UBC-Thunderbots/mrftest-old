#ifndef AI_ROBOT_CONTROLLER_FUZZY_CONTROLLER_H
#define AI_ROBOT_CONTROLLER_FUZZY_CONTROLLER_H

#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include <glibmm.h>
#include <map>

namespace AI {
	namespace RC {
		class FuzzyController : public OldRobotController, public TunableController {
			public:
				void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);

				void clear();

				RobotControllerFactory &get_factory() const;

				FuzzyController(AI::RC::W::World &world, AI::RC::W::Player::Ptr player);

				void set_params(const std::vector<double> &params) {
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
				static const std::vector<double> param_min;
				static const std::vector<double> param_max;
				static const std::vector<double> param_default;

				std::vector<double> param;
		};
	}
}

#endif

