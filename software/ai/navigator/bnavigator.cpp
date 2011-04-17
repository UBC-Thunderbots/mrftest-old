#include "ai/navigator/navigator.h"
#include "util/time.h"
#include "geom/util.h"
#include "ai/navigator/util.h"
#include <iostream>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Flags;
using namespace AI::Nav::Util;
using namespace std;

namespace {
	class BNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();
			Point correct_regional_flags(Point dest, World &world, Player::Ptr player);
			pair<Point, int> check_mobile_violation(Point p, World &world, Player::Ptr player);

		private:
			BNavigator(World &world);
	};

	class BNavigatorFactory : public NavigatorFactory {
		public:
			Navigator::Ptr create_navigator(World &world) const;
			BNavigatorFactory();
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

	BNavigatorFactory::BNavigatorFactory() : NavigatorFactory("BNavigator") {
	}

	Navigator::Ptr BNavigatorFactory::create_navigator(World &world) const {
		return BNavigator::create(world);
	}

	void BNavigator::tick() {
		// init world & field info
		// unused for now, may come in handy
		// const Field &field = world.field();
		// const Ball &ball = world.ball();
		FriendlyTeam &fteam = world.friendly_team();

		// player info
		Player::Ptr player;
		// unsigned int flags;
		Player::Path path;
		Point currentPos, destinationPos, stepPos, closest;
		double currentOri, destinationOri, stepOri;
		timespec ts_next;

		for (std::size_t i = 0; i < fteam.size(); i++) {
			// clear prev path
			path.clear();
			player = fteam.get(i);
			// init player data
			currentPos = player->position();
			currentOri = player->orientation();
			destinationPos = player->destination().first;
			destinationOri = player->destination().second;
			stepPos = currentPos;
			stepOri = currentOri;
			pair<Point, int> collision;
			vector<Point> collision_boundaries;
			// main loop
			int ebreak = 0;
			while ((destinationPos - stepPos).len() > 0.25) {
				// include emergency break just in case
				if (ebreak > 100) {
					break;
				}
				stepPos = stepPos + (destinationPos - stepPos).norm() * 0.25;
				closest = currentPos;
				if (!path.empty()) {
					closest = path.back().first.first;
				}
				// correct for regional rules, apparently these arent that important so can be overriden
				stepPos = correct_regional_flags(stepPos, world, player);
				// check for 'mobile' rules (other robots) these are more important
				collision = check_mobile_violation(stepPos, world, player);
				if (collision.second != 0) {
					// ball flag, navigate around ball
					if (collision.second == 1) {
						collision_boundaries = circle_boundaries(collision.first, 0.5, 16);
					}
					// player
					else if (collision.second == 2) {
						collision_boundaries = circle_boundaries(collision.first, 0.25, 16);
					}
					// find closest point
					for (std::size_t j = 0; j < collision_boundaries.size(); j++) {
						// cout << collision_boundaries[i] << endl;
						if ((collision_boundaries[j] - closest).lensq() < (closest - stepPos).len() && (collision_boundaries[j] - closest).lensq() > 0.1) {
							closest = collision_boundaries[j];
							collision_boundaries.erase(collision_boundaries.begin() + j);
						}
					}
					stepPos = closest;
				}
				stepOri = (closest - stepPos).orientation();
				ts_next = get_next_ts(world.monotonic_time(), closest, stepPos, Point(2, 0));
				path.push_back(make_pair(make_pair(stepPos, stepOri), ts_next));
				ebreak++;
			}
			ts_next = get_next_ts(world.monotonic_time(), closest, stepPos, player->target_velocity());
			path.push_back(make_pair(make_pair(destinationPos, destinationOri), ts_next));
			player->path(path);
		}
	}

	// returns point and 0 if no violation, 1 if ball violation, or 2 if robot
	pair<Point, int> BNavigator::check_mobile_violation(Point p, World &world, Player::Ptr player) {
		const Ball &ball = world.ball();
		unsigned int flags = player->flags();
		if (flags & FLAG_AVOID_BALL_STOP) {
			double C_BALL_STOP = player->MAX_RADIUS + 0.50;
			if ((ball.position() - p).len() < C_BALL_STOP) {
				return make_pair(ball.position(), 1);
			}
		}
		if (flags & FLAG_AVOID_BALL_TINY) {
			double C_BALL_TINY = 0.15;
			if ((ball.position() - p).len() < C_BALL_TINY) {
				return make_pair(ball.position(), 1);
			}
		}
		Player::Ptr friendly;
		Robot::Ptr enemy;
		for (std::size_t i = 0; i < world.friendly_team().size(); i++) {
			friendly = world.friendly_team().get(i);
			if (player != friendly && (p - friendly->position()).len() < player->MAX_RADIUS * 2) {
				return make_pair(friendly->position(), 2);
			}
		}
		for (std::size_t i = 0; i < world.enemy_team().size(); i++) {
			enemy = world.enemy_team().get(i);
			if ((p - enemy->position()).len() < player->MAX_RADIUS * 2) {
				return make_pair(enemy->position(), 2);
			}
		}
		return make_pair(p, 0);
	}

	// correct for regional flags given a point, world and flags
	Point BNavigator::correct_regional_flags(Point dest, World &world, Player::Ptr player) {
		const double ROBOT_MARGIN = 0.0;
		const Field &field = world.field();
		double length, width;
		length = field.length();
		width = field.width();
		Point corrected = dest;
		unsigned int flags = player->flags();
		if (flags & FLAG_CLIP_PLAY_AREA) {
			double C_PLAY_AREA = player->MAX_RADIUS + ROBOT_MARGIN;
			Point play_sw(-length / 2, -width / 2);
			Rect play_area(play_sw, length, width);
			play_area.expand(-C_PLAY_AREA);
			if (!play_area.point_inside(dest)) {
				corrected = clip_point(dest, play_area.nw_corner(), play_area.se_corner());
			}
		}
		if (flags & FLAG_STAY_OWN_HALF) {
			double C_OWN_HALF = player->MAX_RADIUS + ROBOT_MARGIN;
			if (dest.x > 0 - C_OWN_HALF) {
				corrected.x = 0;
			}
		}
		double defense_area_stretch, defense_area_radius;
		defense_area_stretch = field.defense_area_stretch();
		defense_area_radius = field.defense_area_radius();
		if (flags & FLAG_AVOID_FRIENDLY_DEFENSE) {
			double C_FRIENDLY_DEFENSE = defense_area_radius + player->MAX_RADIUS + ROBOT_MARGIN;
			Point top_sw(-length / 2, defense_area_stretch / 2);
			Point mid_sw(-length / 2, -defense_area_stretch / 2);
			Point btm_sw(-length / 2, -((defense_area_stretch / 2) + defense_area_radius));
			Rect top(top_sw, C_FRIENDLY_DEFENSE, C_FRIENDLY_DEFENSE);
			Rect mid(mid_sw, C_FRIENDLY_DEFENSE, defense_area_stretch);
			Rect btm(btm_sw, C_FRIENDLY_DEFENSE, C_FRIENDLY_DEFENSE);
			if (top.point_inside(dest) && (dest - top_sw).len() < C_FRIENDLY_DEFENSE) {
				corrected = top_sw + (dest - top_sw).norm() * (C_FRIENDLY_DEFENSE);
			} else if (mid.point_inside(dest)) {
				corrected.x = -(length / 2) + C_FRIENDLY_DEFENSE;
			} else if (btm.point_inside(dest) && (dest - mid_sw).len() < C_FRIENDLY_DEFENSE) {
				corrected = mid_sw + (dest - mid_sw).norm() * (C_FRIENDLY_DEFENSE);
			}
		}
		if (flags & FLAG_AVOID_ENEMY_DEFENSE) {
			double C_ENEMY_DEFENSE = defense_area_radius + player->MAX_RADIUS + 0.20 + ROBOT_MARGIN;
			Point top_sw((length / 2) - C_ENEMY_DEFENSE, defense_area_stretch / 2);
			Point mid_sw((length / 2) - C_ENEMY_DEFENSE, -defense_area_stretch / 2);
			Point btm_sw((length / 2) - C_ENEMY_DEFENSE, -(defense_area_stretch / 2) - C_ENEMY_DEFENSE);
			Rect top(top_sw, C_ENEMY_DEFENSE, C_ENEMY_DEFENSE);
			Rect mid(mid_sw, C_ENEMY_DEFENSE, defense_area_stretch);
			Rect btm(btm_sw, C_ENEMY_DEFENSE, C_ENEMY_DEFENSE);
			if (top.point_inside(dest) && (dest - top.se_corner()).len() < C_ENEMY_DEFENSE) {
				corrected = top.se_corner() - (top.se_corner() - dest).norm() * (C_ENEMY_DEFENSE);
			} else if (mid.point_inside(dest)) {
				corrected.x = (length / 2) - C_ENEMY_DEFENSE;
			} else if (btm.point_inside(dest) && (dest - mid.se_corner()).len() < C_ENEMY_DEFENSE) {
				corrected = mid.se_corner() - (mid.se_corner() - dest).norm() * (C_ENEMY_DEFENSE);
			}
		}
		if (flags & FLAG_PENALTY_KICK_FRIENDLY) {
			double C_PENALTY_KICK = player->MAX_RADIUS + 0.40 + ROBOT_MARGIN;
			Point f_penalty_mark((-length / 2) + defense_area_stretch, 0);
			if ((dest - f_penalty_mark).len() < C_PENALTY_KICK) {
				corrected = f_penalty_mark + (dest - f_penalty_mark).norm() * C_PENALTY_KICK;
			}
		}
		if (flags & FLAG_PENALTY_KICK_ENEMY) {
			// todo add check for goalie so it doesn't apply
			double C_PENALTY_KICK = player->MAX_RADIUS + 0.40 + ROBOT_MARGIN;
			Point e_penalty_mark((length / 2) - defense_area_stretch, 0);
			if ((dest - e_penalty_mark).len() < C_PENALTY_KICK) {
				corrected = e_penalty_mark - (e_penalty_mark - dest).norm() * C_PENALTY_KICK;
			}
		}
		return corrected;
	}
}

