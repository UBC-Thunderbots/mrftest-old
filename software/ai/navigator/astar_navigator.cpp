#include "ai/hl/util.h"
#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "geom/angle.h"
#include "util/byref.h"
#include "util/time.h"
#include <glibmm.h>
#include <set>
#include <utility>
#include <vector>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::Util;
using namespace AI::Nav::W;
using namespace Glib;

namespace {
	const double INF = 10e9;

	// the degrees where we consider the angle to
	// be virtually the same
	const double ANGLE_TOL = 7.0;

	class PathPoint : public ByRef {
		public:
			typedef ::RefPtr<PathPoint> Ptr;
			Point xy;
			// double orientation;
			PathPoint::Ptr parent;
			double g;
			bool closed;

			PathPoint() : closed(false), g(INF), parent() {
				xy.x = xy.y = 0.0;
			}

			PathPoint(Point x_y) : xy(x_y), closed(false), g(INF), parent() {
			}

			~PathPoint() {
			}

			std::vector<PathPoint::Ptr> getParents() {
				std::vector<PathPoint::Ptr> p;
				PathPoint::Ptr cur = parent;
				while (cur.is()) {
					p.push_back(cur);
					cur = cur->parent;
				}
				std::reverse(p.begin(), p.end());
				return p;
			}
	};

	PathPoint::Ptr start;
	PathPoint::Ptr end;

	bool cmp_f(PathPoint::Ptr a, PathPoint::Ptr b) {
		return a->g + (a->xy - end->xy).len() < b->g + (b->xy - end->xy).len();
	}

	bool cmp_e(PathPoint::Ptr a, PathPoint::Ptr b) {
		return (a->xy - end->xy).len() < (b->xy - end->xy).len();
	}

	bool cmp_p(Player::Ptr a, Player::Ptr b) {
		return a->prio() < b->prio();
	}

	std::set<PathPoint::Ptr, bool (*)(PathPoint::Ptr, PathPoint::Ptr)> open_set(&cmp_f);

	/**
	 * Basic navigator just to play around with
	 */
	class AstarNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			void tick();
			static Navigator::Ptr create(World &world);

		private:
			/**
			 * Constructs a new Navigator.
			 *
			 * \param[in] world the World in which to navigate.
			 */
			AstarNavigator(AI::Nav::W::World &world);
	};

	class AstarNavigatorFactory : public NavigatorFactory {
		public:
			AstarNavigatorFactory();
			~AstarNavigatorFactory();
			Navigator::Ptr create_navigator(AI::Nav::W::World &world) const;
	};

	AstarNavigatorFactory factory_instance;

	NavigatorFactory &AstarNavigator::factory() const {
		return factory_instance;
	}

	AstarNavigatorFactory::AstarNavigatorFactory() : NavigatorFactory("A* Navigator") {
	}

	AstarNavigatorFactory::~AstarNavigatorFactory() {
	}


	Navigator::Ptr AstarNavigator::create(World &world) {
		const Navigator::Ptr p(new AstarNavigator(world));
		return p;
	}
	Navigator::Ptr AstarNavigatorFactory::create_navigator(World &world) const {
		return AstarNavigator::create(world);
	}

	AstarNavigator::AstarNavigator(World &w) : Navigator(w) {
	}

	void AstarNavigator::tick() {
		FriendlyTeam &fteam = world.friendly_team();
		Player::Ptr player;
		std::vector<PathPoint::Ptr> search_space;
		Player::Path path;

		std::vector<Player::Ptr> players;

		for (unsigned int i = 0; i < fteam.size(); i++) {
			players.push_back(fteam.get(i));
		}
		std::sort(players.begin(), players.end(), cmp_p);

		for (unsigned int i = 0; i < fteam.size(); i++) {
			player = players[i];
			PathPoint::Ptr player_start(new PathPoint(player->position()));
			PathPoint::Ptr player_end(new PathPoint(player->destination().first));
			std::vector<PathPoint::Ptr> add_on;
			unsigned int added_flags = 0;

			start = player_start;
			end = player_end;

			if (player->type() == AI::Flags::MOVE_CATCH) {
				added_flags = AI::Flags::FLAG_AVOID_BALL_TINY;
				double target_minus = player->MAX_RADIUS + Ball::RADIUS + 0.07;
				Point p(target_minus, 0);
				p = p.rotate(player->destination().second);
				Point ball_catch = world.ball().position();
				PathPoint::Ptr p_end(new PathPoint(ball_catch - p));
				end = p_end;
				PathPoint::Ptr p_add(new PathPoint(world.ball().position()));
				add_on.push_back(p_add);
			}

		if (player->type() == AI::Flags::MOVE_RAM_BALL) {
			Point start = player->position();
			Point end = player->destination().first;

		}

			path.clear();
			start->g = 0.0;

			if (valid_path(player->position(), end->xy, world, player, added_flags)) {
				if (player->type() == AI::Flags::MOVE_CATCH) {
					Point diff = world.ball().position() - player->position();
					diff = diff.rotate(-player->orientation());
					double ang = std::max(radians2degrees(diff.orientation()), radians2degrees(angle_diff(player->orientation(), player->destination().second)));
					if (ang < 0) {
						ang = -ang;
					}
					if (ang < ANGLE_TOL) {
						path.push_back(std::make_pair(std::make_pair(world.ball().position(), player->destination().second), world.monotonic_time()));
						player->path(path);
						continue;
					}
				}

				path.push_back(std::make_pair(std::make_pair(end->xy, player->destination().second), world.monotonic_time()));
				for (std::vector<PathPoint::Ptr>::iterator it = add_on.begin(); it != add_on.end(); it++) {
					path.push_back(std::make_pair(std::make_pair((*it)->xy, player->destination().second), world.monotonic_time()));
				}

				player->path(path);
				continue;
			}

			search_space.clear();
			open_set.clear();
			std::vector<Point> p = get_obstacle_boundaries(world, player, added_flags);
			std::vector<Point> end_near = get_destination_alternatives(end->xy, world, player);
			for (std::vector<Point>::iterator it = end_near.begin(); it != end_near.end(); it++) {
				p.push_back(*it);
			}
			search_space.push_back(start);
			search_space.push_back(end);
			for (std::vector<Point>::const_iterator it = p.begin(); it != p.end(); it++) {
				PathPoint::Ptr temp(new PathPoint(*it));
				search_space.push_back(temp);
			}

			open_set.insert(start);
			bool ans = false;
			while (!open_set.empty()) {
				PathPoint::Ptr cur = *(open_set.begin());
				open_set.erase(open_set.begin());
				if (cur == end) {
					std::vector<PathPoint::Ptr> p = cur->getParents();
					for (std::vector<PathPoint::Ptr>::iterator it = p.begin(); it != p.end(); it++) {
						if (*it != start) {
							path.push_back(std::make_pair(std::make_pair((*it)->xy, player->destination().second), world.monotonic_time()));
						}
					}
					path.push_back(std::make_pair(std::make_pair(cur->xy, player->destination().second), world.monotonic_time()));

					for (std::vector<PathPoint::Ptr>::iterator it = add_on.begin(); it != add_on.end(); it++) {
						path.push_back(std::make_pair(std::make_pair((*it)->xy, player->destination().second), world.monotonic_time()));
					}

					ans = true;
					player->path(path);
					break;
				}
				cur->closed = true;
				for (std::vector<PathPoint::Ptr>::const_iterator it = search_space.begin(); it != search_space.end(); it++) {
					if (!(*it)->closed && valid_path(cur->xy, (*it)->xy, world, player, added_flags)) {
						double g = cur->g + (cur->xy - (*it)->xy).len();
						open_set.insert(*it);
						if (g < (*it)->g) {
							(*it)->g = g;
							(*it)->parent = cur;
						}
					}
				}
			}
			if (!ans) {
				sort(search_space.begin(), search_space.end(), cmp_e);
				for (std::vector<PathPoint::Ptr>::const_iterator it1 = search_space.begin(); it1 != search_space.end(); it1++) {
					PathPoint::Ptr cur = *it1;
					std::vector<PathPoint::Ptr> p = cur->getParents();
					if (p.size() <= 0) {
						continue;
					}
					for (std::vector<PathPoint::Ptr>::const_iterator it = p.begin(); it != p.end(); it++) {
						if (*it != start) {
							path.push_back(std::make_pair(std::make_pair((*it)->xy, player->destination().second), world.monotonic_time()));
						}
					}
					path.push_back(std::make_pair(std::make_pair(cur->xy, player->destination().second), world.monotonic_time()));
					ans = true;
					player->path(path);
					break;
				}
			}
			if (!ans) {
				path.push_back(std::make_pair(std::make_pair(player->position(), player->destination().second), world.monotonic_time()));
				player->path(path);
			}
		}
	}
}

