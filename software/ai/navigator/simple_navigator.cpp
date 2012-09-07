#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;

namespace {
	/**
	 * Simple Navigator
	 * The main functions that are required to be able to implement a navigator.
	 */
	class SimpleNavigator : public Navigator {
		public:
			SimpleNavigator(World &world);
			void tick();
			NavigatorFactory &factory() const;
	};
}

SimpleNavigator::SimpleNavigator(World &world) : Navigator(world) {
}

void SimpleNavigator::tick() {
	FriendlyTeam &fteam = world.friendly_team();
	timespec ts;

	Player::Ptr player;
	Player::Path path;

	Point currentPosition, destinationPosition;
	Angle currentOrientation, destinationOrientation;

	for (std::size_t i = 0; i < fteam.size(); i++) {
		path.clear();
		player = fteam.get(i);
		currentPosition = player->position();
		currentOrientation = player->orientation();
		destinationPosition = player->destination().first;
		destinationOrientation = player->destination().second;

		ts = get_next_ts(world.monotonic_time(), currentPosition, destinationPosition, player->target_velocity());

		path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), ts));
		player->path(path);
	}
}

NAVIGATOR_REGISTER(SimpleNavigator)

