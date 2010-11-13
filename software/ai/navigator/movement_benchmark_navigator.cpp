#include "ai/hl/util.h"
#include "ai/navigator/navigator.h"
#include "util/time.h"
#include <iostream>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;

namespace {
	/**
	 * Movement Benchmark
	 * Used for tuning robot controller.
	 */
	class MovementBenchmarkNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();

		private:
			MovementBenchmarkNavigator(World &world);
			~MovementBenchmarkNavigator();
	};

	class MovementBenchmarkNavigatorFactory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			MovementBenchmarkNavigatorFactory();
			~MovementBenchmarkNavigatorFactory();
	};

	MovementBenchmarkNavigatorFactory simple_nav_factory;

	NavigatorFactory &MovementBenchmarkNavigator::factory() const {
		return simple_nav_factory;
	}

	Navigator::Ptr MovementBenchmarkNavigator::create(World &world) {
		const Navigator::Ptr p(new MovementBenchmarkNavigator(world));
		return p;
	}

	MovementBenchmarkNavigator::MovementBenchmarkNavigator(World &world) : Navigator(world) {
	}

	MovementBenchmarkNavigator::~MovementBenchmarkNavigator() {
	}

	MovementBenchmarkNavigatorFactory::MovementBenchmarkNavigatorFactory() : NavigatorFactory("TEST: Movement Benchmark") {
	}

	MovementBenchmarkNavigatorFactory::~MovementBenchmarkNavigatorFactory() {
	}

	Navigator::Ptr MovementBenchmarkNavigatorFactory::create_navigator(World &world) const {
		return MovementBenchmarkNavigator::create(world);
	}
	
	const double PI = M_PI;
	int taskIndex = 0;
	int numTasks = 10;
	
	const std::pair<Point, double> tasks[] =
	{
		std::make_pair(Point(1.2, 0), 0),
		std::make_pair(Point(0.5, 0), PI),
		std::make_pair(Point(2.5, 0), 0),
		std::make_pair(Point(0.5, 1.2), PI),
		std::make_pair(Point(1, -0.6), 0),
		std::make_pair(Point(2, 0.6), PI/2),
		std::make_pair(Point(1, -0.6), -PI/2),
		std::make_pair(Point(0.5, 0), 0),
		std::make_pair(Point(2.5, 0.6), -PI/2),
		std::make_pair(Point(1.2, 0), 0)
	};


	void MovementBenchmarkNavigator::tick() {
		const Field &field = world.field();
		FriendlyTeam &fteam = world.friendly_team();
		
		if (fteam.size() != 1) {
			std::cerr << "error: must have only 1 robot in the team!" << std::endl;
			return;
		}

		Player::Ptr player;
		std::vector<std::pair<std::pair<Point, double>, timespec> > path;

		Point currentPosition, destinationPosition;
		double currentOrientation, destinationOrientation;

		path.clear();
		player = fteam.get(0);
		currentPosition = player->position();
		currentOrientation = player->orientation();
		if ((currentPosition-tasks[taskIndex].first).len() < 0.2 && player->velocity().len() < 0.05) {
			taskIndex++;
			if (taskIndex == numTasks) taskIndex = 0;
		}

		path.push_back(std::make_pair(std::make_pair(tasks[taskIndex].first, tasks[taskIndex].second), world.monotonic_time()));
		player->path(path);
	}
}

