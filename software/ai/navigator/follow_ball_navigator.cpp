#include "ai/navigator/navigator.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;

namespace {
	/**
	 * Follow Ball Navigator
	 * Used for testing robot controllers to match the ball's velocity.
	 */
	class FollowBallNavigator : public Navigator {
		public:
			explicit FollowBallNavigator(World world);
			void tick();
			NavigatorFactory &factory() const;
	};
}

FollowBallNavigator::FollowBallNavigator(World world) : Navigator(world) {
}

void FollowBallNavigator::tick() {
	for (Player player : world.friendly_team()) {
		Point currentPosition = player.position();
		Point diff = (world.ball().position() - currentPosition);
		Point destinationPosition = world.ball().position() - 0.3 * (diff / diff.len());
		Angle destinationOrientation = (world.ball().position() - currentPosition).orientation();

		Player::Path path;
		path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
		path.push_back(std::make_pair(std::make_pair(destinationPosition + world.ball().velocity(), destinationOrientation), world.monotonic_time()));
		player.path(path);
	}
}

NAVIGATOR_REGISTER(FollowBallNavigator)

