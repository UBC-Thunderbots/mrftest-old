#include "ai/hl/util.h"
#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "util/time.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::Util;
using namespace AI::Nav::W;

namespace {
	const double STEP_DISTANCE = 0.9;

	const double ROTATE_STEP = M_PI / 32.0;

	/**
	 * Reactive Navigator
	 * Built as a navigator comparable to
	 * 2010 singapore version
	 * built for comparison to other navigators
	 * i.e. useful for benchmarking
	 */
	class ReactiveNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();

		private:
			ReactiveNavigator(World &world);
			~ReactiveNavigator();
	};

	class ReactiveNavigatorFactory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			ReactiveNavigatorFactory();
			~ReactiveNavigatorFactory();
	};

	ReactiveNavigatorFactory nav_factory;

	NavigatorFactory &ReactiveNavigator::factory() const {
		return nav_factory;
	}

	Navigator::Ptr ReactiveNavigator::create(World &world) {
		const Navigator::Ptr p(new ReactiveNavigator(world));
		return p;
	}

	ReactiveNavigator::ReactiveNavigator(World &world) : Navigator(world) {
	}

	ReactiveNavigator::~ReactiveNavigator() {
	}

	ReactiveNavigatorFactory::ReactiveNavigatorFactory() : NavigatorFactory("Reactive Navigator") {
	}

	ReactiveNavigatorFactory::~ReactiveNavigatorFactory() {
	}

	Navigator::Ptr ReactiveNavigatorFactory::create_navigator(World &world) const {
		return ReactiveNavigator::create(world);
	}

	void ReactiveNavigator::tick() {
		const Field &field = world.field();
		FriendlyTeam &fteam = world.friendly_team();

		Player::Ptr player;
		Player::Path path;

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
				}
				path.push_back(std::make_pair(std::make_pair(currentPosition + add, destinationOrientation), world.monotonic_time()));
				player->path(path);
			} else {
				path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), world.monotonic_time()));
				player->path(path);
			}
		}
	}
}

