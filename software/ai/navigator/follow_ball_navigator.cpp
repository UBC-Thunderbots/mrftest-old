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
			FollowBallNavigator(World world);
			void tick();
			NavigatorFactory &factory() const;
	};
}

FollowBallNavigator::FollowBallNavigator(World world) : Navigator(world) {
}

void FollowBallNavigator::tick() {
	FriendlyTeam fteam = world.friendly_team();

	Player player;
	Player::Path path;

	Point currentPosition, destinationPosition;
	Angle currentOrientation, destinationOrientation;

	for (std::size_t i = 0; i < fteam.size(); i++) {
		path.clear();
		player = fteam.get(i);
		currentPosition = player.position();
		currentOrientation = player.orientation();
		Point diff = (world.ball().position() - currentPosition);
		destinationPosition = world.ball().position() - 0.3 * (diff / diff.len());
		destinationOrientation = (world.ball().position() - currentPosition).orientation();

		path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
		path.push_back(std::make_pair(std::make_pair(destinationPosition + world.ball().velocity(), destinationOrientation), world.monotonic_time()));
		player.path(path);
	}
}

NAVIGATOR_REGISTER(FollowBallNavigator)

