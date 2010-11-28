#include "ai/hl/util.h"
#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "util/time.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::Util;
using namespace AI::Nav::W;

namespace {
	const double STEP_DISTANCE = 1.0;

	const double ROTATE_STEP = M_PI / 32.0;

	/**
	 * Lazy Navigator
	 * This is a modified version of the reactive navigator.
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

	LazyNavigatorFactory nav_factory;

	NavigatorFactory &LazyNavigator::factory() const {
		return nav_factory;
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
				Point vec = (destinationPosition - currentPosition);
				Point add(0, 0);
				if (vec.lensq() > 0.01) {
					vec = STEP_DISTANCE * vec.norm();
					double rotate = ROTATE_STEP;
					for (int i = 0; (i * rotate) < M_PI / 2; i++) {
						Point left = vec.rotate(i * rotate);
						Point right = vec.rotate(-i * rotate);
						if (valid_path(currentPosition, currentPosition + right, world, player)) {
							add = right;
							break;
						}
						if (valid_path(currentPosition, currentPosition + left, world, player)) {
							add = left;
							break;
						}
					}
					if (add.x == vec.x && add.y == vec.y) {
						// Do binary search to find the closest intersection point.
						Point vector = destinationPosition-currentPosition;
                        double min = 0;
                        double max = 1;
                        while (max-min > 0.01) {
							double mid = (min+max)/2.0;
							if (valid_path(currentPosition, currentPosition + (mid * vector), world, player)) {
								min = mid;
							} else {
								max = mid;
							}
                        }
                        double mid = (min+max)/2.0;
                        path.push_back(std::make_pair(std::make_pair(currentPosition + (mid * vector), destinationOrientation), world.monotonic_time()));
                        player->path(path);
					} else {
						path.push_back(std::make_pair(std::make_pair(currentPosition+add, destinationOrientation), world.monotonic_time()));
						player->path(path);
					}
				} else {
					path.push_back(std::make_pair(std::make_pair(currentPosition, destinationOrientation), world.monotonic_time()));
					player->path(path);
				}
			} else {
				path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
				player->path(path);
			}
		}
	}
}

