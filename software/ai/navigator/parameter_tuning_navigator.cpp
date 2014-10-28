#include "ai/navigator/navigator.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "util/stochastic_local_search.h"
#include <iostream>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using AI::RC::TunableController;
using namespace AI::Nav::W;

namespace {
	constexpr double pos_dis_threshold = 0.1;
	constexpr double pos_vel_threshold = 0.05;
	constexpr Angle ori_dis_threshold = Angle::of_radians(0.175);
	constexpr Angle ori_vel_threshold = Angle::of_radians(0.05);

	/**
	 * Parameter Tuning
	 * Used for tuning robot controllers.
	 */
	class ParameterTuningNavigator final : public Navigator {
		public:
			explicit ParameterTuningNavigator(World world);
			void tick() override;
			NavigatorFactory &factory() const override;

		private:
			StochasticLocalSearch sls;
	};

	constexpr double PI = M_PI;
	int taskIndex = 0;
	constexpr int numTasks = 15;
	int time = 0;
	constexpr int limit = 1000;
	int best = limit;

	const std::pair<Point, Angle> tasks[] = {
		std::make_pair(Point(1.2, 0), Angle::of_radians(0)),
		std::make_pair(Point(1.5, 0), Angle::of_radians(-PI / 2)),
		std::make_pair(Point(1.2, 0.3), Angle::of_radians(0)),
		std::make_pair(Point(1.2, -0.3), Angle::of_radians(PI)),
		std::make_pair(Point(1.2, 0), Angle::of_radians(0)),
		std::make_pair(Point(1.2, -0.3), Angle::of_radians(PI)),
		std::make_pair(Point(1.2, 0), Angle::of_radians(0)),
		std::make_pair(Point(0.5, 0), Angle::of_radians(PI)),
		std::make_pair(Point(2.5, 0), Angle::of_radians(0)),
		std::make_pair(Point(0.5, 1.2), Angle::of_radians(PI)),
		std::make_pair(Point(1, -0.6), Angle::of_radians(0)),
		std::make_pair(Point(2, 0.6), Angle::of_radians(PI / 2)),
		std::make_pair(Point(1, -0.6), Angle::of_radians(-PI / 2)),
		std::make_pair(Point(0.5, 0), Angle::of_radians(0)),
		std::make_pair(Point(2.5, 0.6), Angle::of_radians(-PI / 2))
	};
}

ParameterTuningNavigator::ParameterTuningNavigator(World world) : Navigator(world), sls(TunableController::get_instance()->get_params_default(), TunableController::get_instance()->get_params_min(), TunableController::get_instance()->get_params_max()) {
}

void ParameterTuningNavigator::tick() {
	FriendlyTeam fteam = world.friendly_team();

	TunableController *tc = TunableController::get_instance();

	if (fteam.size() != 1) {
		std::cerr << "error: must have only 1 robot in the team!" << std::endl;
		return;
	}

	time++;

	Player player;
	Player::Path path;

	path.clear();
	player = fteam[0];
	Point currentPosition = player.position();
	if ((currentPosition - tasks[taskIndex].first).len() < pos_dis_threshold
		&& player.velocity().len() < pos_vel_threshold
		&& tasks[taskIndex].second.angle_diff(player.orientation()) < ori_dis_threshold
		&& player.avelocity() < ori_vel_threshold) {
		taskIndex++;
		if (taskIndex == 1) {
			time = 0;
		}
	}

	if (taskIndex == numTasks || time >= best) {
		taskIndex = 0;
		std::cout << "Parameters: ";
		std::vector<double> params = tc->get_params();
		for (std::vector<double>::size_type i = 0; i < params.size(); i++) {
			std::cout << params[i] << " ";
		}
		std::cout << "Time steps taken: " << time;
		if (time < best) {
			best = time;
			sls.set_cost(time);
		} else {
			sls.set_cost(limit);
		}
		sls.hill_climb();
		std::cout << " Best parameters: ";
		params = sls.get_best_params();
		for (std::vector<double>::size_type i = 0; i < params.size(); i++) {
			std::cout << params[i] << " ";
		}
		std::cout << std::endl;
		tc->set_params(sls.get_params());
		time = 0;
	}

	path.push_back(std::make_pair(std::make_pair(tasks[taskIndex].first, tasks[taskIndex].second), world.monotonic_time()));
	player.path(path);
}

NAVIGATOR_REGISTER(ParameterTuningNavigator)

