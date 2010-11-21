#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/time.h"
#include <glibmm.h>
#include <vector>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;

namespace {
	class BNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			static Navigator::Ptr create(World &world);
			void tick();

			Point correct_regional_flags(Point dest, World &world, Player::Ptr player);
			bool check_mobile_violation(Point p, World &world, Player::Ptr player);
			bool check_regional_violation(Point p, World &world, Player::Ptr player);

			Point getVector(Point start, Point goal, Player::Ptr player);

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
	BNavigator::BNavigator(World &world) : Navigator(world) {}
	BNavigator::~BNavigator() {}
	BNavigatorFactory::BNavigatorFactory() : NavigatorFactory("BNavigator") {}
	BNavigatorFactory::~BNavigatorFactory() {}
	Navigator::Ptr BNavigatorFactory::create_navigator(World &world) const {
		return BNavigator::create(world);
	}
	Point BNavigator::getVector(Point start, Point goal, Player::Ptr player) {
		return start + (goal - start).norm() * player->MAX_RADIUS;
	}

	void BNavigator::tick() {
		const Field &field = world.field();
		const Ball &ball = world.ball();
		FriendlyTeam &fteam = world.friendly_team();
		Player::Ptr player;
		std::vector<std::pair<std::pair<Point, double>, timespec>> path;
		std::vector<Point> rpath;
		Point currp, destp, nextp;
		double curro, desto, nexto;
		unsigned int flags;
		std::vector<Point> boundaries, valid_boundaries;

		for (unsigned int i = 0; i < fteam.size(); i++) {
			path.clear();
			rpath.clear();
			player = fteam.get(i);
			currp = player->position();
			destp = player->destination().first;
			curro = player->orientation();
			desto = player->destination().second;
			flags = player->flags();

			nextp = getVector(currp, destp, player);
			nextp = correct_regional_flags(nextp, world, player);

			boundaries = get_obstacle_boundaries(world, player);

			for (unsigned int i = 0; i < boundaries.size(); i++) {
				if (!check_regional_violation(boundaries[i], world, player)) {
					valid_boundaries.push_back(boundaries[i]);
				}
			}

			for (int i = 0; i < 25; i++) {
				nextp = getVector(currp, destp, player);
				nextp = correct_regional_flags(nextp, world, player);
				if (check_mobile_violation(nextp, world, player)) {
					Point closest(1e99, 1e99);
					// closest = valid_boundaries[0];
					for (unsigned int i = 1; i < valid_boundaries.size(); i++) {
						if ((valid_boundaries[i] - nextp).len() < (closest - nextp).len()) {
							closest = valid_boundaries[i];
							valid_boundaries.erase(valid_boundaries.begin() + i);
						}
					}
					nextp = closest;
				}
				nexto = (currp - nextp).orientation();
				path.push_back(std::make_pair(std::make_pair(nextp, nexto), world.monotonic_time()));
				rpath.push_back(nextp);
				currp = nextp;
			}
			player->path(path);
		}
	}

	bool BNavigator::check_regional_violation(Point p, World &world, Player::Ptr player) {
		const double ROBOT_MARGIN = 0.0;
		const Field &field = world.field();
		double length, width;
		length = field.length();
		width = field.width();
		unsigned int flags = player->flags();
		if (flags & FLAG_CLIP_PLAY_AREA) {
			double C_PLAY_AREA = player->MAX_RADIUS + ROBOT_MARGIN;
			Point play_sw(-length / 2, -width / 2);
			Rect play_area(play_sw, length, width);
			play_area.expand(-C_PLAY_AREA);
			if (!play_area.point_inside(p)) {
				return true;
			}
		}
		if (flags & FLAG_STAY_OWN_HALF) {
			double C_OWN_HALF = player->MAX_RADIUS + ROBOT_MARGIN;
			if (p.x > 0 - C_OWN_HALF) {
				return true;
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
			if (top.point_inside(p) && (p - top_sw).len() < C_FRIENDLY_DEFENSE) {
				return true;
			} else if (mid.point_inside(p)) {
				return true;
			} else if (btm.point_inside(p) && (p - mid_sw).len() < C_FRIENDLY_DEFENSE) {
				return true;
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
			if (top.point_inside(p) && (p - top.se_corner()).len() < C_ENEMY_DEFENSE) {
				return true;
			} else if (mid.point_inside(p)) {
				return true;
			} else if (btm.point_inside(p) && (p - mid.se_corner()).len() < C_ENEMY_DEFENSE) {
				return true;
			}
		}
		if (flags & FLAG_PENALTY_KICK_FRIENDLY) {
			double C_PENALTY_KICK = player->MAX_RADIUS + 0.40 + ROBOT_MARGIN;
			Point f_penalty_mark((-length / 2) + defense_area_stretch, 0);
			if ((p - f_penalty_mark).len() < C_PENALTY_KICK) {
				return true;
			}
		}
		if (flags & FLAG_PENALTY_KICK_ENEMY) {
			double C_PENALTY_KICK = player->MAX_RADIUS + 0.40 + ROBOT_MARGIN;
			Point e_penalty_mark((length / 2) - defense_area_stretch, 0);
			if ((p - e_penalty_mark).len() < C_PENALTY_KICK) {
				return true;
			}
		}
		return false;
	}

	bool BNavigator::check_mobile_violation(Point p, World &world, Player::Ptr player) {
		const Ball &ball = world.ball();
		unsigned int flags = player->flags();
		if (flags & FLAG_AVOID_BALL_STOP) {
			double C_BALL_STOP = player->MAX_RADIUS + 0.50;
			if ((ball.position() - p).len() < C_BALL_STOP) {
				return true;
			}
		}
		if (flags & FLAG_AVOID_BALL_TINY) {
			double C_BALL_TINY = 0.15;
			if ((ball.position() - p).len() < C_BALL_TINY) {
				return true;
			}
		}
		Player::Ptr friendly;
		Robot::Ptr enemy;
		for (unsigned int i = 0; i < world.friendly_team().size(); i++) {
			friendly = world.friendly_team().get(i);
			if (player != friendly && (p - friendly->position()).len() < player->MAX_RADIUS * 2) {
				return true;
			}
		}
		for (unsigned int i = 0; i < world.enemy_team().size(); i++) {
			enemy = world.enemy_team().get(i);
			if ((p - enemy->position()).len() < player->MAX_RADIUS * 2) {
				return true;
			}
		}
		return false;
	}

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
			double C_PENALTY_KICK = player->MAX_RADIUS + 0.40 + ROBOT_MARGIN;
			Point e_penalty_mark((length / 2) - defense_area_stretch, 0);
			if ((dest - e_penalty_mark).len() < C_PENALTY_KICK) {
				corrected = e_penalty_mark - (e_penalty_mark - dest).norm() * C_PENALTY_KICK;
			}
		}
		return corrected;
	}
}

