#include "ai/hl/util.h"
#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "util/time.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::Util;
using namespace AI::Nav::W;

namespace {
	/**
	 * Lazy Navigator
	 * A work in progress.
	 */
	class LazyNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();

		private:
			LazyNavigator(World &world);
			~LazyNavigator();
	};

	class LazyNavigatorFactory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			LazyNavigatorFactory();
			~LazyNavigatorFactory();
	};

	LazyNavigatorFactory simple_nav_factory;

	NavigatorFactory &LazyNavigator::factory() const {
		return simple_nav_factory;
	}

	Navigator::Ptr LazyNavigator::create(World &world) {
		const Navigator::Ptr p(new LazyNavigator(world));
		return p;
	}

	LazyNavigator::LazyNavigator(World &world) : Navigator(world) {
	}

	LazyNavigator::~LazyNavigator() {
	}

	LazyNavigatorFactory::LazyNavigatorFactory() : NavigatorFactory("Lazy Navigator") {
	}

	LazyNavigatorFactory::~LazyNavigatorFactory() {
	}

	Navigator::Ptr LazyNavigatorFactory::create_navigator(World &world) const {
		return LazyNavigator::create(world);
	}

	void LazyNavigator::tick() {
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
			destinationPosition = player->destination().first;
			destinationOrientation = player->destination().second;

			if (!valid_path(currentPosition, destinationPosition, world, player)) {
				// Do binary search:
				Point vector = destinationPosition - currentPosition;
				double min = 0;
				double max = 1;
				while (max - min > 0.01) {
					double mid = (min + max) / 2.0;
					if (valid_path(currentPosition, currentPosition + (mid * vector), world, player)) {
						min = mid;
					} else {
						max = mid;
					}
				}
				double mid = (min + max) / 2.0;

				path.push_back(std::make_pair(std::make_pair(currentPosition + (mid * vector), destinationOrientation), world.monotonic_time()));
				player->path(path);
			} else {
				path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
				player->path(path);
			}
		}
	}
}

