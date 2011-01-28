#include "ai/hl/util.h"
#include "ai/navigator/navigator.h"
#include "util/time.h"

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

	void PivotNavigator::tick() {
		FriendlyTeam &fteam = world.friendly_team();

		Player::Ptr player;
		std::vector<std::pair<std::pair<Point, double>, timespec> > path;

		Point currentPosition, destinationPosition;
		double currentOrientation, destinationOrientation;

		for (unsigned int i = 0; i < fteam.size(); i++) {
			path.clear();
			player = fteam.get(i);
			currentPosition = player->position();
			currentOrientation = player->orientation();
			
			// tunable magic numbers BEWARE!!!
			double offset_angle = 35.0;
			double offset_distance = 1.0;
			double orientation_offset = 40.0;
			
			destinationPosition = player->position() + Point(offset_distance*cos(player->orientation()-offset_angle*M_PI/180.0), offset_distance*sin(player->orientation()-offset_angle*M_PI/180.0));
			destinationOrientation = player->orientation() + orientation_offset*M_PI/180.0;
			
			path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
			player->path(path);
		}
	}
}

