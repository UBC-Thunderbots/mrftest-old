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
	class SimpleNavigator final : public Navigator {
		public:
			explicit SimpleNavigator(World world);
			void tick() override;
			NavigatorFactory &factory() const override;
	};
}

SimpleNavigator::SimpleNavigator(World world) : Navigator(world) {
}

void SimpleNavigator::tick() {
	for (Player player : world.friendly_team()) {
		Point currentPosition = player.position();
		Point destinationPosition = player.destination().first;
		Angle destinationOrientation = player.destination().second;

		AI::Timestamp ts = get_next_ts(world.monotonic_time(), currentPosition, destinationPosition, player.target_velocity());

		Player::Path path;
		path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), ts));
		player.path(path);
	}
}

NAVIGATOR_REGISTER(SimpleNavigator)

