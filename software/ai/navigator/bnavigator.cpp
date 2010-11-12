#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "util/dprint.h"
#include "util/time.h"
#include <algorithm>
#include <cmath>
#include <glibmm.h>
#include <iostream>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;

namespace {
	const double MAX_SPEED = 2.0;
	const double SLOW_DIST = 0.75;

	class BNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();

			Point calcArrival(Point x, Point y);
			Point correct_for_flags(Point x,const Field &field, unsigned int flags);
			
			bool check_dest_valid1(Point dest,World &world, Player::Ptr player);

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
		const Ball &ball = world.ball();
		FriendlyTeam &fteam = world.friendly_team();
		Player::Ptr player;
		std::vector<std::pair<std::pair<Point, double>, timespec>> path;
		Point currp, destp, nextp;
		double curro, desto, nexto;
		timespec ts;
		unsigned int flags;

		for (unsigned int i = 0; i < fteam.size(); i++) {
			
			player = fteam.get(i);
			currp = player->position();
			destp = player->destination().first;
			curro = player->orientation();
			desto = player->destination().second;
			flags = player->flags();

			nextp = calcArrival(currp, destp);
							
			nextp = correct_for_flags(nextp, field, flags);
			nexto = (nextp-currp).orientation();

			if ((destp - nextp).len() > (destp - currp).len()) {
				nextp = destp;
				nexto = desto;
			}

			ts = world.monotonic_time();
			//ts.tv_sec += (nextp - currp).len() / player->avelocity();
			path.push_back(std::make_pair(std::make_pair(nextp, nexto), ts));
			player->path(path);
			path.clear();
			if( check_dest_valid(ball.position(), world, player) ) LOG_WARN("TRUE");
			else LOG_WARN("FALSE");
		}
	}

	Point BNavigator::calcArrival(Point x, Point y) {
		Point offset = y - x;
		double distance = offset.len();
		double ramped_speed = MAX_SPEED * (distance / SLOW_DIST);
		double clipped_speed = std::min(ramped_speed, MAX_SPEED);
		Point desired = (clipped_speed / distance) * offset;
		//return desired;
		return x+(y-x).norm()*(0.25);
	}


	bool BNavigator::check_dest_valid1(Point dest,World &world, Player::Ptr player) {
		double AVOID_TINY_CONSTANT = 0.15; // constant for AVOID_BALL_TINY
		double AVOID_PENALTY_CONSTANT = 0.40; // constant for AVOID_PENALTY_*
		const Field &field = world.field();
		double length, width;
		length = field.length();
		width = field.width();
		unsigned int flags = player->flags();
		// check for CLIP_PLAY_AREA simple rectangular bounds
		if ((flags & FLAG_CLIP_PLAY_AREA) == FLAG_CLIP_PLAY_AREA) {
			if (dest.x < -length/2
				|| dest.x > length/2
				|| dest.y < -width/2
				|| dest.y > width/2) return false;
		}
		// check for STAY_OWN_HALF
		if ((flags & FLAG_STAY_OWN_HALF) == FLAG_STAY_OWN_HALF) {
			if (dest.y > 0) return false;
		}
		/* field information for AVOID_*_DEFENSE_AREA checks
		 * defined by defense_area_radius distance from two goal posts
		 * and rectangular stretch directly in front of goal
		 */
		Point f_post1, f_post2, e_post1, e_post2;
		double defense_area_stretch, defense_area_radius;
		defense_area_stretch = field.defense_area_stretch();
		defense_area_radius = field.defense_area_radius();
		f_post1 = Point(-length/2, defense_area_stretch/2);
		f_post2 = Point(-length/2, -defense_area_stretch/2);
		// friendly case
		if ((flags & FLAG_AVOID_FRIENDLY_DEFENSE) == FLAG_AVOID_FRIENDLY_DEFENSE) {
			if (dest.x < -(length/2)+defense_area_radius
				&& dest.x > -length/2
				&& dest.y < defense_area_stretch/2
				&& dest.y > -defense_area_stretch/2) return false;
			else if ((dest-f_post1).len() < defense_area_radius
						|| (dest-f_post2).len() < defense_area_radius) return false;
		}
		// enemy goal posts
		e_post1 = Point(length/2, defense_area_stretch/2);
		e_post2 = Point(length/2, -defense_area_stretch/2);
		// enemy case
		if ((flags & FLAG_AVOID_ENEMY_DEFENSE) == FLAG_AVOID_ENEMY_DEFENSE) {
			if (dest.x > length/2-defense_area_radius-0.20
				&& dest.x < length/2
				&& dest.y < defense_area_stretch/2
				&& dest.y > -defense_area_stretch/2) return false;
			else if ((dest-e_post1).len() < defense_area_radius+0.20 // 20cm from defense line
						|| (dest-e_post2).len() < defense_area_radius+0.20) return false;
		}
		// ball checks (lol)
		const Ball &ball = world.ball();
		// AVOID_BALL_STOP
		if ((flags & FLAG_AVOID_BALL_STOP) == FLAG_AVOID_BALL_STOP) {
			if ((dest-ball.position()).len() < 0.5) return false; // avoid ball by 50cm
		}
		// AVOID_BALL_TINY
		if ((flags & FLAG_AVOID_BALL_TINY) == FLAG_AVOID_BALL_TINY) {
			if ((dest-ball.position()).len() < AVOID_TINY_CONSTANT) return false; // avoid ball by this constant?
		}
		/* PENALTY_KICK_* checks
		 * penalty marks are defined 450mm & equidistant from both goal posts,
		 * i assume that defense_area_stretch is a close enough approximation
		 */
		Point f_penalty_mark, e_penalty_mark;
		f_penalty_mark = Point((-length/2)+defense_area_stretch, 0);
		e_penalty_mark = Point((length/2)-defense_area_stretch, 0);
		if ((flags & FLAG_PENALTY_KICK_FRIENDLY) == FLAG_PENALTY_KICK_FRIENDLY) {
			if ((dest-f_penalty_mark).len() < AVOID_PENALTY_CONSTANT) return false; 
		}
		if ((flags & FLAG_PENALTY_KICK_ENEMY) == FLAG_PENALTY_KICK_ENEMY) {
			if ((dest-e_penalty_mark).len() < AVOID_PENALTY_CONSTANT) return false;
		}
		/* check if dest is on another robot
		 * 2*robot radius (maybe we need some sort of margin?) from center to center
		 */
		FriendlyTeam &f_team = world.friendly_team();
		EnemyTeam &e_team = world.enemy_team();
		Player::Ptr f_player;
		Robot::Ptr e_player;
		// friendly case
		for(unsigned int i=0;i<f_team.size();i++) {
			f_player = f_team.get(i);
			if ((dest-f_player->position()).len() < 2*f_player->MAX_RADIUS) return false;
		}
		// enemy case
		for(unsigned int i=0;i<e_team.size();i++) {
			e_player = e_team.get(i);
			if ((dest-e_player->position()).len() < 2*e_player->MAX_RADIUS) return false;
		}
		
		return true;
	}

	Point BNavigator::correct_for_flags(Point next,const Field &field, unsigned int flags) {
		if ((flags & FLAG_CLIP_PLAY_AREA) == FLAG_CLIP_PLAY_AREA) {
			if (next.x > field.length() / 2) {
				next.x = field.length() / 2;
			}
			if (next.x < -field.length() / 2) {
				next.x = -field.length() / 2;
			}
			if (next.y > field.width() / 2) {
				next.y = field.width() / 2;
			}
			if (next.y < -field.width() / 2) {
				next.y = -field.width() / 2;
			}
		}
		
		if ((flags & FLAG_STAY_OWN_HALF) == FLAG_STAY_OWN_HALF) {
			if (next.y > 0) {
				next.y = 0;
			}
		}

		if ((flags & FLAG_AVOID_FRIENDLY_DEFENSE) == FLAG_AVOID_FRIENDLY_DEFENSE) {
			Point post1, post2;
			post1 = Point(-(field.length()/2),field.defense_area_stretch()/2);
			post2 = Point(-(field.length()/2),-(field.defense_area_stretch()/2));
			
			if(next.x < -(field.length()/2)+field.defense_area_radius()
			&& next.x > -(field.length()/2)
			&& next.y < (field.defense_area_stretch()/2)
			&& next.y > -(field.defense_area_stretch()/2)) {
				next.x = -(field.length()/2)+field.defense_area_radius();
			} else if((next-post1).len()<field.defense_area_radius() || (next-post2).len()<field.defense_area_radius()) {
				if((next-post1).len()<(next-post2).len()) {
					next = post1+field.defense_area_radius()*(next-post1).norm();
				} else {
					next = post2+field.defense_area_radius()*(next-post2).norm();
				}
			}	
		}

		if ((flags & FLAG_AVOID_ENEMY_DEFENSE) == FLAG_AVOID_ENEMY_DEFENSE) {
			Point post1, post2;
			post1 = Point(field.length()/2,field.defense_area_stretch()/2);
			post2 = Point(field.length()/2,-(field.defense_area_stretch()/2));
				
			if(next.x > field.length()/2-field.defense_area_radius()
			&& next.x < field.length()/2
			&& next.y < (field.defense_area_stretch()/2)
			&& next.y > -(field.defense_area_stretch()/2)) {
				next.x = field.length()/2-field.defense_area_radius()-0.2;
			} else if((next-post1).len()<field.defense_area_radius() || (next-post2).len()<field.defense_area_radius()) {
				if((next-post1).len()<(next-post2).len()) {
					next = post1+(0.2+field.defense_area_radius())*(next-post1).norm();
				} else {
					next = post2+(0.2+field.defense_area_radius())*(next-post2).norm();
				}
			}	
		}
		return next;
	}

}

