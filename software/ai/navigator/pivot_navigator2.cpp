#include "ai/navigator/navigator.h"
#include "geom/angle.h"
#include "util/time.h"
#include "util/param.h"
#include <cmath>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;

namespace {
	/**
	 * Pivot Navigator
	 * The main functions that are required to be able to implement a navigator.
	 */
	class PivotNavigator2 : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();

		private:
			PivotNavigator2(World &world);
	};

	class PivotNavigator2Factory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			PivotNavigator2Factory();
	};

	PivotNavigator2Factory pivot_nav_factory;

	NavigatorFactory &PivotNavigator2::factory() const {
		return pivot_nav_factory;
	}

	Navigator::Ptr PivotNavigator2::create(World &world) {
		const Navigator::Ptr p(new PivotNavigator2(world));
		return p;
	}

	PivotNavigator2::PivotNavigator2(World &world) : Navigator(world) {
	}

	PivotNavigator2Factory::PivotNavigator2Factory() : NavigatorFactory("TEST: Pivot Navigator 2") {
	}

	Navigator::Ptr PivotNavigator2Factory::create_navigator(World &world) const {
		return PivotNavigator2::create(world);
	}
	
	DoubleParam offset_angle("offset angle (degrees)", "Nav/Pivot2", 30.0, -1000.0, 1000.0);
	DoubleParam offset_distance("offset distance", "Nav/Pivot2", 0.15, -10.0, 10.0);
	DoubleParam orientation_offset("orientation offset (degrees)", "Nav/Pivot2", 30.0, -1000.0, 1000.0);

	void PivotNavigator2::tick() {
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

			Point diff = (world.ball().position() - currentPosition).rotate(offset_angle * M_PI / 180.0);

			destinationPosition = world.ball().position() - offset_distance * diff.norm();
			destinationOrientation = (world.ball().position() - currentPosition).orientation() + orientation_offset * M_PI / 180.0;

			path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));

			player->path(path);
		}
	}
}

