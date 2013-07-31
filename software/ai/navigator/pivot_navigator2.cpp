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
	class PivotNavigator2 : public Navigator {
		public:
			PivotNavigator2(World world);
			void tick();
			NavigatorFactory &factory() const;
	};

	DegreeParam offset_angle("offset angle (degrees)", "Nav/Pivot2", 30.0, -1000.0, 1000.0);
	DoubleParam offset_distance("offset distance", "Nav/Pivot2", 0.15, -10.0, 10.0);
	DegreeParam orientation_offset("orientation offset (degrees)", "Nav/Pivot2", 30.0, -1000.0, 1000.0);
}

PivotNavigator2::PivotNavigator2(World world) : Navigator(world) {
}

void PivotNavigator2::tick() {
	for (Player player : world.friendly_team()) {
		Point currentPosition = player.position();
		Angle currentOrientation = player.orientation();

		Point diff = (world.ball().position() - currentPosition).rotate(offset_angle);

		Point destinationPosition = world.ball().position() - offset_distance * diff.norm();
		Angle destinationOrientation = (world.ball().position() - currentPosition).orientation() + orientation_offset;

		Player::Path path;
		path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
		player.path(path);
	}
}

NAVIGATOR_REGISTER(PivotNavigator2)

