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
			~PivotNavigator2();
	};

	class PivotNavigator2Factory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			PivotNavigator2Factory();
			~PivotNavigator2Factory();
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

	PivotNavigator2::~PivotNavigator2() {
	}

	PivotNavigator2Factory::PivotNavigator2Factory() : NavigatorFactory("TEST: Pivot Navigator Aram") {
	}

	PivotNavigator2Factory::~PivotNavigator2Factory() {
	}

	Navigator::Ptr PivotNavigator2Factory::create_navigator(World &world) const {
		return PivotNavigator2::create(world);
	}
	
	DoubleParam pivot_radius("Pivot Navigator A: pivot radius", 1.0, 0.0, 10.0);
	DoubleParam acceleration("Pivot Navigator A: max acceleration", 10.0, 0.0, 1000.0);

	void PivotNavigator2::tick() {
		FriendlyTeam &fteam = world.friendly_team();

		Player::Ptr player;
		Player::Path path;

		Point currentPosition, currentVelocity, destinationPosition, targetPosition, turnCentre, diff;
		double currentOrientation, destinationOrientation, turnRadius;

		for (std::size_t i = 0; i < fteam.size(); i++) {
			path.clear();
			player = fteam.get(i);
			currentPosition = player->position();
			currentVelocity = player->velocity();
			currentOrientation = player->orientation();
			
			if (currentVelocity.lensq() < 0.0001)
				currentVelocity = Point(0.0, 0.01).rotate(currentOrientation);
			
			diff = world.ball().position() - currentPosition;
			turnRadius = currentVelocity.lensq()/acceleration;
			
			targetPosition = world.ball().position() + diff; // try to reach opposite side of ball
			//targetPosition = Point(0, 0); // try to reach centre of field
			
			if (diff.cross(currentVelocity) > 0)
				turnCentre = currentPosition + turnRadius*currentVelocity.rotate(M_PI/2.0);
			else
				turnCentre = currentPosition + turnRadius*currentVelocity.rotate(-M_PI/2.0);
			
			if ((world.ball().position() - turnCentre).len() > turnRadius + pivot_radius)
				destinationPosition = targetPosition;
			else if (diff.len() > pivot_radius)
				destinationPosition = turnCentre + turnRadius*currentVelocity.norm();
			else
				destinationPosition = 2*turnCentre - currentPosition;
			//if ((world.ball().position() - destinationPosition).len() < pivot_radius)
				//destinationPosition = world.ball().position() + pivot_radius*(destinationPosition - world.ball().position()).norm();
			
			destinationOrientation = (world.ball().position() - destinationPosition).orientation();

			path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));

			player->path(path);
		}
	}
}

