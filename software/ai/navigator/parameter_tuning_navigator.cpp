#include "ai/navigator/navigator.h"
#include "util/time.h"
#include "geom/angle.h"
#include "ai/robot_controller/tunable_controller.h"
#include "util/stochastic_local_search.h"
#include <iostream>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using AI::RC::TunableController;
using namespace AI::Nav::W;

namespace {

	const double pos_dis_threshold = 0.1;
	const double pos_vel_threshold = 0.05;
	const double ori_dis_threshold = 0.175;
	const double ori_vel_threshold = 0.05;

	/**
	 * Parameter Tuning
	 * Used for tuning robot controllers.
	 */
	class ParameterTuningNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();

		private:
			ParameterTuningNavigator(World &world);
			StochasticLocalSearch *sls;
			~ParameterTuningNavigator();
	};

	class ParameterTuningNavigatorFactory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			ParameterTuningNavigatorFactory();
			~ParameterTuningNavigatorFactory();
	};

	ParameterTuningNavigatorFactory simple_nav_factory;

	NavigatorFactory &ParameterTuningNavigator::factory() const {
		return simple_nav_factory;
	}

	Navigator::Ptr ParameterTuningNavigator::create(World &world) {
		const Navigator::Ptr p(new ParameterTuningNavigator(world));
		return p;
	}

	ParameterTuningNavigator::ParameterTuningNavigator(World &world) : Navigator(world) {
		TunableController *tc = TunableController::get_instance();
		sls = new StochasticLocalSearch(tc->get_params_default(), tc->get_params_min(), tc->get_params_max());
	}

	ParameterTuningNavigator::~ParameterTuningNavigator() {
	}

	ParameterTuningNavigatorFactory::ParameterTuningNavigatorFactory() : NavigatorFactory("TEST: Parameter Tuning") {
	}

	ParameterTuningNavigatorFactory::~ParameterTuningNavigatorFactory() {
	}

	Navigator::Ptr ParameterTuningNavigatorFactory::create_navigator(World &world) const {
		return ParameterTuningNavigator::create(world);
	}

	const double PI = M_PI;
	int taskIndex = 0;
	int numTasks = 15;
	int time = 0;
	int limit = 1000;
	int best = limit;

	const std::pair<Point, double> tasks[] = {
		std::make_pair(Point(1.2, 0), 0),
		std::make_pair(Point(1.5, 0), -PI / 2),
		std::make_pair(Point(1.2, 0.3), 0),
		std::make_pair(Point(1.2, -0.3), PI),
		std::make_pair(Point(1.2, 0), 0),
		std::make_pair(Point(1.2, -0.3), PI),
		std::make_pair(Point(1.2, 0), 0),
		std::make_pair(Point(0.5, 0), PI),
		std::make_pair(Point(2.5, 0), 0),
		std::make_pair(Point(0.5, 1.2), PI),
		std::make_pair(Point(1, -0.6), 0),
		std::make_pair(Point(2, 0.6), PI / 2),
		std::make_pair(Point(1, -0.6), -PI / 2),
		std::make_pair(Point(0.5, 0), 0),
		std::make_pair(Point(2.5, 0.6), -PI / 2)
	};


	void ParameterTuningNavigator::tick() {
		FriendlyTeam &fteam = world.friendly_team();

		TunableController *tc = TunableController::get_instance();

		if (fteam.size() != 1) {
			std::cerr << "error: must have only 1 robot in the team!" << std::endl;
			return;
		}

		time++;

		Player::Ptr player;
		Player::Path path;

		path.clear();
		player = fteam.get(0);
		Point currentPosition = player->position();
		if ((currentPosition - tasks[taskIndex].first).len() < pos_dis_threshold 
				&& player->velocity().len() < pos_vel_threshold
				&& angle_diff(tasks[taskIndex].second, player->orientation()) < ori_dis_threshold
				&& player->avelocity() < ori_vel_threshold) {
			taskIndex++;
			if (taskIndex == 1) {
				time = 0;
			}
		}

		if (taskIndex == numTasks || time >= best) {
			taskIndex = 0;
			std::cout << "Parameters: ";
			std::vector<double> params = tc->get_params();
			for (uint i = 0; i < params.size(); i++) {
				std::cout << params[i] << " ";
			}
			std::cout << "Time steps taken: " << time;
			if (time < best) {
				best = time;
				sls->set_cost(time);
			} else {
				sls->set_cost(limit);
			}
			sls->hill_climb();
			std::cout << " Best parameters: ";
			params = sls->get_best_params();
			for (uint i = 0; i < params.size(); i++) {
				std::cout << params[i] << " ";
			}
			std::cout << std::endl;
			tc->set_params(sls->get_params());
			time = 0;
		}

		path.push_back(std::make_pair(std::make_pair(tasks[taskIndex].first, tasks[taskIndex].second), world.monotonic_time()));
		player->path(path);
	}
}

