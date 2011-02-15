#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "util/dprint.h"
#include <cmath>
#include <glibmm.h>
#include <map>
#include <algorithm>
#include <iostream>
#include "uicomponents/param.h"

using AI::RC::RobotController;
using AI::RC::TunableController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	const int P = 2;

	// 0 max distance for linear control (outside of this range is just fast )
	// 1 amount to scale linear speed factor by
	// 2 amount to scale both factors by
	const double ANGLE_TOL = 0.05;
	const double DIST_TOL = 0.05;

	DoubleParam proportion(" amount to scale controller velocities by ", 2.0, 0.1, 10.0);

	const double arr_min[P] = { 3.0, 0.5};
	const double arr_max[P] = { 8.0, 10.0};

	// robot parameters
	const double arr_def[P] = { 4.72052, 3.0};

	// simulator parameters
	// const double arr_def[P] = { 8.71043, 1.95671, 1.08009, 4.59125, 9.40896 };


	const std::vector<double> param_min(arr_min, arr_min + P);
	const std::vector<double> param_max(arr_max, arr_max + P);
	const std::vector<double> param_default(arr_def, arr_def + P);

	class CircleController : public RobotController, public TunableController {
		public:
			CircleController(World &world, Player::Ptr player) : RobotController(world, player), param(param_default) {
			}

			void tick() {
				const Player::Path &path = player->path();
				if (path.empty()) {
					return;
				}

				double orientation_diff = angle_mod(path[0].first.second -  player->orientation());
				Point location_diff = (path[0].first.first - player->position());
				//				std::cout<<location_diff<<std::endl;
				Point centre_of_line = (path[0].first.first + player->position())/2.0;

				Point robot_vel = location_diff;
				double robot_ang_vel = orientation_diff;

				bool angle_change = orientation_diff > ANGLE_TOL || orientation_diff < -ANGLE_TOL;
				if(location_diff.len() > DIST_TOL && angle_change){
					//			std::cout<<robot_vel<<' '<<robot_ang_vel;
					Point direction = location_diff.norm();
					double distance_to_cover = location_diff.len();

					double dir = 1.0;
					if(orientation_diff <0){
						dir = -1.0;
					}
					double distance_from_line = 0.5*location_diff.len()/tan(orientation_diff/2.0);

					//			std::cout<<distance_from_line<< ' '<< location_diff<<' '<<orientation_diff<< std::endl;
					// vector taking the centre of the line seg (dest - cur_location)
					// to the centre of the pivot point
					Point line_to_centre = dir*distance_from_line*((location_diff.norm()).rotate(M_PI/2.0));
					Point pivot_centre = centre_of_line + line_to_centre;				
					double pivot_radius = (pivot_centre - player->position()).len();
					
					distance_to_cover = orientation_diff*pivot_radius;

					//					std::cout<<distance_to_cover<<' ';
					direction = ((player->position() - pivot_centre).rotate(dir*M_PI/2.0)).norm();
					//					std::cout<<direction<<std::endl;

					robot_vel = distance_to_cover*direction; 
					//				std::cout<<' '<<robot_vel<<' '<<robot_ang_vel<<std::endl;
				}

				robot_vel*=proportion;
				robot_ang_vel*=proportion;

				timespec cur_time = world.monotonic_time();

				//			if(robot_vel.len() > DIST_TOL){
					//					double speed = std::min(5.0, robot_vel.len());
					//					robot_vel = speed*(robot_vel.norm());
				//				}

				//	if(path[0].second

				int wheel_speeds[4] = { 0, 0, 0, 0 };
				//		convert_to_wheels(location_diff,0.0, wheel_speeds);
					convert_to_wheels(robot_vel,robot_ang_vel, wheel_speeds);

				player->drive(wheel_speeds);
			}

			void set_params(const std::vector<double> &params) {
				this->param = params;
			}

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
			std::vector<double> param;
	};

	class CircleControllerFactory : public RobotControllerFactory {
		public:
			CircleControllerFactory() : RobotControllerFactory("circle controller") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr player) const {
				RobotController::Ptr p(new CircleController(world, player));
				return p;
			}
	};

	CircleControllerFactory factory;
}

