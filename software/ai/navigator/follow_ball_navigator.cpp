#include "ai/hl/util.h"
#include "ai/navigator/navigator.h"
#include "util/time.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;

namespace {
	/**
	 * Simple Navigator
	 * The main functions that are required to be able to implement a navigator.
	 */
	class FollowBallNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();

		private:
			FollowBallNavigator(World &world);
			~FollowBallNavigator();
	};

	class FollowBallNavigatorFactory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			FollowBallNavigatorFactory();
			~FollowBallNavigatorFactory();
	};

	FollowBallNavigatorFactory simple_nav_factory;

	NavigatorFactory &FollowBallNavigator::factory() const {
		return simple_nav_factory;
	}

	Navigator::Ptr FollowBallNavigator::create(World &world) {
		const Navigator::Ptr p(new FollowBallNavigator(world));
		return p;
	}

	FollowBallNavigator::FollowBallNavigator(World &world) : Navigator(world) {
	}

	FollowBallNavigator::~FollowBallNavigator() {
	}

	FollowBallNavigatorFactory::FollowBallNavigatorFactory() : NavigatorFactory("Follow Ball Navigator") {
	}

	FollowBallNavigatorFactory::~FollowBallNavigatorFactory() {
	}

	Navigator::Ptr FollowBallNavigatorFactory::create_navigator(World &world) const {
		return FollowBallNavigator::create(world);
	}

	void FollowBallNavigator::tick() {
		const Field &field = world.field();
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
			Point diff = (world.ball().position() - currentPosition);
			destinationPosition = world.ball().position() - 0.3*(diff / diff.len());
			destinationOrientation = (world.ball().position() - currentPosition).orientation();

			path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
			path.push_back(std::make_pair(std::make_pair(destinationPosition+world.ball().velocity(), destinationOrientation), world.monotonic_time()));
			player->path(path);
		}
	}
}

