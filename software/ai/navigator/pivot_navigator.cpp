#include "ai/navigator/navigator.h"
#include "geom/angle.h"
#include "geom/param.h"
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
	class PivotNavigator : public Navigator {
		public:
			PivotNavigator(World world);
			void tick();
			NavigatorFactory &factory() const;
	};

	DegreeParam offset_angle(u8"offset angle (degrees)", u8"Nav/Pivot", 80.0, -1000.0, 1000.0);
	DoubleParam offset_distance(u8"offset distance", u8"Nav/Pivot", 0.1, -1000.0, 10.0);
	DegreeParam orientation_offset(u8"orientation offset (degrees)", u8"Nav/Pivot", 30.0, -1000.0, 1000.0);
}

PivotNavigator::PivotNavigator(World world) : Navigator(world) {
}

void PivotNavigator::tick() {
	for (Player player : world.friendly_team()) {
		Point currentPosition = player.position();

		// tunable magic numbers BEWARE!!!
		// double offset_angle = 80.0;
		// double offset_distance = 0.1;
		// double orientation_offset = 30.0;

		Point diff = (world.ball().position() - currentPosition);

		Point destinationPosition;
		Angle destinationOrientation;
		Player::Path path;
		if ((diff.len() < 0.3 && diff.len() > 0.1) && player.velocity().len() < 0.3 && (world.ball().position() - currentPosition).orientation().angle_diff(player.orientation()) < Angle::of_radians(1.2)) {
			destinationPosition = player.position() + offset_distance * Point::of_angle(player.orientation() - offset_angle);
			destinationOrientation = player.orientation() + orientation_offset;
			path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
		} else {
			destinationPosition = world.ball().position() - 0.2 * diff.norm();
			destinationOrientation = (world.ball().position() - currentPosition).orientation();

			path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
		}

		player.path(path);
	}
}

NAVIGATOR_REGISTER(PivotNavigator)

