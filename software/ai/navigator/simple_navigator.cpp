/**
	Simple Navigator
	The main functions that are required to be able to implement a navigator.





**/
#include "ai/navigator/navigator.h"
#include "ai/hl/util.h"
#include <time.h>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;

namespace {
	
	class SimpleNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();
		
		private:
			SimpleNavigator(World &world);
			~SimpleNavigator();
	};

	class SimpleNavigatorFactory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			SimpleNavigatorFactory();
			~SimpleNavigatorFactory();
	};

	SimpleNavigatorFactory simple_nav_factory;

	NavigatorFactory &SimpleNavigator::factory() const {
		return simple_nav_factory;
	}

	Navigator::Ptr SimpleNavigator::create(World &world) {
		const Navigator::Ptr p(new SimpleNavigator(world));
		return p;
	}

	SimpleNavigator::SimpleNavigator(World &world) : Navigator(world) {
	}

	SimpleNavigator::~SimpleNavigator() {
	}

	SimpleNavigatorFactory::SimpleNavigatorFactory() : NavigatorFactory("Simple Navigator") {
	}

	SimpleNavigatorFactory::~SimpleNavigatorFactory() {
	}

	Navigator::Ptr SimpleNavigatorFactory::create_navigator(World &world) const {
		return SimpleNavigator::create(world);
	}

	void SimpleNavigator::tick() {
		const Field *field = &(world.field());
		FriendlyTeam *fteam = &(world.friendly_team());
		
		Player::Ptr player;
		std::vector <std::pair <std::pair <Point, double>, timespec>> path;
		struct timespec ts;

		Point currentPosition, destinationPosition;
		double currentOrientation, destinationOrientation;

		for(unsigned int i=0; i<fteam->size(); i++) {
			player = fteam->get(i);
			currentPosition = player->position();
			currentOrientation = player->orientation();
			destinationPosition = player->destination().first;
			destinationOrientation = player->destination().second;
			
			timespec_now(ts);
			path.push_back(std::make_pair(std::make_pair(destinationPosition, destinationOrientation), ts));
			player->path(path);
		}
	}

}
