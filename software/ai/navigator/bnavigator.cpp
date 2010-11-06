#include "ai/hl/util.h"
#include "ai/navigator/navigator.h"
#include "util/time.h"
#include <iostream>
#include "math.h"
#include <algorithm>
#include "util/dprint.h"
#include <glibmm.h>
		
using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Flags;

namespace {
	const double MAX_SPEED = 2.0;
	const double SLOW_DIST = 0.75;

	class BNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();

			double normalisedAngle(Point x, Point y);
			Point calcArrival(Point x, Point y);

		private:
			BNavigator(World &world);
			~BNavigator();
	};

	class BNavigatorFactory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			BNavigatorFactory();
			~BNavigatorFactory();
	};

	BNavigatorFactory simple_nav_factory;

	NavigatorFactory &BNavigator::factory() const {
		return simple_nav_factory;
	}

	Navigator::Ptr BNavigator::create(World &world) {
		const Navigator::Ptr p(new BNavigator(world));
		return p;
	}

	BNavigator::BNavigator(World &world) : Navigator(world) {
	}

	BNavigator::~BNavigator() {
	}

	BNavigatorFactory::BNavigatorFactory() : NavigatorFactory("BNavigator") {
	}

	BNavigatorFactory::~BNavigatorFactory() {
	}

	Navigator::Ptr BNavigatorFactory::create_navigator(World &world) const {
		return BNavigator::create(world);
	}

	void BNavigator::tick() {
		const Field &field = world.field();
		FriendlyTeam &fteam = world.friendly_team();
		Player::Ptr player;
		std::vector<std::pair<std::pair<Point, double>, timespec>> path;
		Point currp, destp, nextp;
		double curro, desto, nexto;
		timespec ts;
		unsigned int flags;

		for( unsigned int i=0; i<fteam.size(); i++ ) {
			player = fteam.get(i);
			currp = player->position();
			destp = player->destination().first;
			curro = player->orientation();
			desto = player->destination().second;
			flags = player->flags();
			nextp = calcArrival(currp, destp);

			nexto = normalisedAngle(currp, nextp);

			if((destp-nextp).len()>(destp-currp).len()) {
				nextp = destp;
				nexto = desto;
			}

			if((flags & FLAG_CLIP_PLAY_AREA) == FLAG_CLIP_PLAY_AREA) {
				if(nextp.x > field.length()/2) nextp.x = field.length()/2;
				if(nextp.x < -field.length()/2) nextp.x = -field.length()/2;
				if(nextp.y > field.width()/2) nextp.y = field.width()/2;
				if(nextp.y < -field.width()/2) nextp.y = -field.width()/2;
			}

			if((flags & FLAG_STAY_OWN_HALF) == FLAG_STAY_OWN_HALF) {
				if(nextp.y > 0) nextp.y = 0;
			}

			if((flags & FLAG_AVOID_FRIENDLY_DEFENSE) == FLAG_AVOID_FRIENDLY_DEFENSE) {
				const double defense_stretch = world.field().defense_area_stretch();
				const double defense_radius = world.field().defense_area_radius();
				const double field_length = world.field().length();
				const Point pole1 = Point(-field_length, defense_stretch / 2 + defense_radius);
				const Point pole2 = Point(-field_length, -defense_stretch / 2 - defense_radius);
				double dist1 = (nextp - pole1).len();
				double dist2 = (nextp - pole2).len();
				
				if (nextp.x > -field_length / 2 && nextp.x < -field_length / 2 + defense_radius && nextp.y > -defense_stretch / 2 && nextp.y < defense_stretch / 2) {
					nextp.x = -field_length / 2 + defense_radius;
					if(abs(-defense_stretch/2-nextp.y)>abs(defense_stretch/2-nextp.y)) {
						nextp.y = -defense_stretch/2;
					} else {
						nextp.y = defense_stretch/2;
					}
				}
				
				if (dist1 < defense_radius || dist2 < defense_radius) {
				
				}
		}



			ts = world.monotonic_time();
			ts.tv_sec += (nextp-currp).len() / player->avelocity();

			path.push_back(std::make_pair(std::make_pair(nextp, nexto), ts));
			player->path(path);
			path.clear();
		}
	}

	double BNavigator::normalisedAngle(Point x, Point y) {
		double angle = (M_PI/2) - atan2(y.x-x.x, y.y-x.y);
		if( angle > M_PI ) angle -= 2*M_PI;
		if( angle < -M_PI ) angle += 2*M_PI;
		return angle;
	}

	Point BNavigator::calcArrival(Point x, Point y) {
		Point offset = y-x;
		double distance = offset.len();
		double ramped_speed = MAX_SPEED*(distance/SLOW_DIST);
		double clipped_speed = std::min(ramped_speed, MAX_SPEED);
		Point desired = (clipped_speed/distance)*offset;
		return desired;
	}


}

