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
	class PivotNavigatorAram : public Navigator {
		public:
			PivotNavigatorAram(World world);
			void tick();
			NavigatorFactory &factory() const;
	};

	DoubleParam pivot_radius("pivot radius", "Nav/PivotAram", 1.0, 0.0, 10.0);
	DoubleParam acceleration("max acceleration", "Nav/PivotAram", 10.0, 0.0, 1000.0);
}

PivotNavigatorAram::PivotNavigatorAram(World world) : Navigator(world) {
}

void PivotNavigatorAram::tick() {
	FriendlyTeam fteam = world.friendly_team();

	Player::Ptr player;
	Player::Path path;

	Point currentPosition, currentVelocity, destinationPosition, targetPosition, turnCentre, diff;
	Angle currentOrientation, destinationOrientation;
	double turnRadius;

	for (std::size_t i = 0; i < fteam.size(); i++) {
		path.clear();
		player = fteam.get(i);
		currentPosition = player->position();
		currentVelocity = player->velocity();
		currentOrientation = player->orientation();

		if (currentVelocity.lensq() < 0.0001) {
			currentVelocity = Point(0.0, 0.01).rotate(currentOrientation);
		}

		diff = world.ball().position() - currentPosition;
		turnRadius = currentVelocity.lensq() / acceleration;

		targetPosition = world.ball().position() + diff; // try to reach opposite side of ball
		// targetPosition = Point(0, 0); // try to reach centre of field

		if (diff.cross(currentVelocity) > 0) {
			turnCentre = currentPosition + turnRadius * currentVelocity.rotate(Angle::QUARTER);
		} else {
			turnCentre = currentPosition + turnRadius * currentVelocity.rotate(-Angle::QUARTER);
		}

		if ((world.ball().position() - turnCentre).len() > turnRadius + pivot_radius) {
			destinationPosition = targetPosition;
		} else if (diff.len() > pivot_radius) {
			destinationPosition = turnCentre + turnRadius * currentVelocity.norm();
		} else {
			destinationPosition = 2 * turnCentre - currentPosition;
		}
		// if ((world.ball().position() - destinationPosition).len() < pivot_radius)
		// destinationPosition = world.ball().position() + pivot_radius*(destinationPosition - world.ball().position()).norm();

		destinationOrientation = (world.ball().position() - destinationPosition).orientation();

		path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));

		player->path(path);
	}
}

NAVIGATOR_REGISTER(PivotNavigatorAram)

