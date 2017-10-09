#include "ai/navigator/navigator.h"
#include "geom/angle.h"
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
	class PivotNavigatorAram final : public Navigator {
		public:
			explicit PivotNavigatorAram(World world);
			void tick() override;
			NavigatorFactory &factory() const override;
	};

	DoubleParam pivot_radius(u8"pivot radius", u8"AI/Nav/PivotAram", 1.0, 0.0, 10.0);
	DoubleParam acceleration(u8"max acceleration", u8"AI/Nav/PivotAram", 10.0, 0.0, 1000.0);
}

PivotNavigatorAram::PivotNavigatorAram(World world) : Navigator(world) {
}

void PivotNavigatorAram::tick() {
	for (Player player : world.friendly_team()) {
		Point currentPosition = player.position();
		Point currentVelocity = player.velocity();
		Angle currentOrientation = player.orientation();

		if (currentVelocity.lensq() < 0.0001) {
			currentVelocity = Point(0.0, 0.01).rotate(currentOrientation);
		}

		Point diff = world.ball().position() - currentPosition;
		double turnRadius = currentVelocity.lensq() / acceleration;

		Point targetPosition = world.ball().position() + diff; // try to reach opposite side of ball
		// targetPosition = Point(0, 0); // try to reach centre of field

		Point turnCentre;
		if (diff.cross(currentVelocity) > 0) {
			turnCentre = currentPosition + turnRadius * currentVelocity.rotate(Angle::quarter());
		} else {
			turnCentre = currentPosition + turnRadius * currentVelocity.rotate(-Angle::quarter());
		}

		Point destinationPosition;
		if ((world.ball().position() - turnCentre).len() > turnRadius + pivot_radius) {
			destinationPosition = targetPosition;
		} else if (diff.len() > pivot_radius) {
			destinationPosition = turnCentre + turnRadius * currentVelocity.norm();
		} else {
			destinationPosition = 2 * turnCentre - currentPosition;
		}
		// if ((world.ball().position() - destinationPosition).len() < pivot_radius)
		// destinationPosition = world.ball().position() + pivot_radius*(destinationPosition - world.ball().position()).norm();

		Angle destinationOrientation = (world.ball().position() - destinationPosition).orientation();

		Player::Path path;
		path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
		player.path(path);
	}
}

NAVIGATOR_REGISTER(PivotNavigatorAram)

