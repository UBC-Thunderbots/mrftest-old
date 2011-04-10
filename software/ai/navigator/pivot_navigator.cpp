#include "ai/navigator/navigator.h"
#include "geom/angle.h"
#include "util/param.h"
#include "util/time.h"
#include <cmath>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;

namespace {
	/**
	 * Pivot Navigator
	 * The main functions that are required to be able to implement a navigator.
	 */
	class PivotNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();

		private:
			PivotNavigator(World &world);
			~PivotNavigator();
	};

	class PivotNavigatorFactory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			PivotNavigatorFactory();
			~PivotNavigatorFactory();
	};

	PivotNavigatorFactory pivot_nav_factory;

	NavigatorFactory &PivotNavigator::factory() const {
		return pivot_nav_factory;
	}

	Navigator::Ptr PivotNavigator::create(World &world) {
		const Navigator::Ptr p(new PivotNavigator(world));
		return p;
	}

	PivotNavigator::PivotNavigator(World &world) : Navigator(world) {
	}

	PivotNavigator::~PivotNavigator() {
	}

	PivotNavigatorFactory::PivotNavigatorFactory() : NavigatorFactory("TEST: Pivot Navigator") {
	}

	PivotNavigatorFactory::~PivotNavigatorFactory() {
	}

	Navigator::Ptr PivotNavigatorFactory::create_navigator(World &world) const {
		return PivotNavigator::create(world);
	}
	
	DoubleParam offset_angle("Pivot Navigator: offset angle", 80.0, -1000.0, 1000.0);
	DoubleParam offset_distance("Pivot Navigator: offset distance", 0.1, -1000.0, 10.0);
	DoubleParam orientation_offset("Pivot Navigator: orientation offset", 30.0, -1000.0, 1000.0);

	void PivotNavigator::tick() {
		FriendlyTeam &fteam = world.friendly_team();

		Player::Ptr player;
		Player::Path path;

		Point currentPosition, destinationPosition;
		double currentOrientation, destinationOrientation;

		for (std::size_t i = 0; i < fteam.size(); i++) {
			path.clear();
			player = fteam.get(i);
			currentPosition = player->position();
			currentOrientation = player->orientation();

			// tunable magic numbers BEWARE!!!
			//double offset_angle = 80.0;
			//double offset_distance = 0.1;
			//double orientation_offset = 30.0;

			Point diff = (world.ball().position() - currentPosition);

			if ((diff.len() < 0.3 && diff.len() > 0.1) && player->velocity().len() < 0.3 && angle_diff((world.ball().position() - currentPosition).orientation(), player->orientation()) < 1.2) {
				destinationPosition = player->position() + Point(offset_distance * std::cos(player->orientation() - offset_angle * M_PI / 180.0), offset_distance * std::sin(player->orientation() - offset_angle * M_PI / 180.0));
				destinationOrientation = player->orientation() + orientation_offset * M_PI / 180.0;
				path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
			} else {
				destinationPosition = world.ball().position() - 0.2 * diff.norm();
				destinationOrientation = (world.ball().position() - currentPosition).orientation();

				path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
			}

			player->path(path);
		}
	}
}

