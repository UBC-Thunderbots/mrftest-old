#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "geom/angle.h"
#include "util/time.h"
#include <memory>
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
	const Angle ANGLE_TOL = Angle::of_degrees(7.0);

	class PathPoint {
		public:
			typedef std::shared_ptr<PathPoint> Ptr;
			Point xy;
			// double orientation;
			PathPoint::Ptr parent;
			double g;
			bool closed;

			PathPoint() : g(INF), closed(false) {
				xy.x = xy.y = 0.0;
			}

			PathPoint(Point x_y) : xy(x_y), g(INF), closed(false) {
			}

			std::vector<PathPoint::Ptr> getParents() {
				std::vector<PathPoint::Ptr> p;
				PathPoint::Ptr cur = parent;
				while (cur) {
					p.push_back(cur);
					cur = cur->parent;
				}
				std::reverse(p.begin(), p.end());
				return p;
			}
	};

	// bad to be here
	PathPoint::Ptr start, end;

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
			Navigator::Ptr create_navigator(AI::Nav::W::World &world) const;
	};

	AstarNavigatorFactory factory_instance;

	NavigatorFactory &AstarNavigator::factory() const {
		return factory_instance;
	}

	AstarNavigatorFactory::AstarNavigatorFactory() : NavigatorFactory("A* Navigator") {
	}

	Navigator::Ptr AstarNavigator::create(World &world) {
		Navigator::Ptr p(new AstarNavigator(world));
		return p;
	}
	Navigator::Ptr AstarNavigatorFactory::create_navigator(World &world) const {
		return AstarNavigator::create(world);
	}

	AstarNavigator::AstarNavigator(World &w) : Navigator(w) {
	}

	void AstarNavigator::tick() {
		auto cmp_f = [](PathPoint::Ptr a, PathPoint::Ptr b) { return a->g + (a->xy - end->xy).len() < b->g + (b->xy - end->xy).len(); };
		std::set<PathPoint::Ptr, decltype(cmp_f)> open_set(cmp_f);

		FriendlyTeam &fteam = world.friendly_team();
		Player::Ptr player;
		std::vector<PathPoint::Ptr> search_space;
		Player::Path path;

		std::vector<Player::Ptr> players;

		for (std::size_t i = 0; i < fteam.size(); i++) {
			players.push_back(fteam.get(i));
		}
		std::sort(players.begin(), players.end(), [](Player::Ptr a, Player::Ptr b) { return a->prio() < b->prio(); });

		for (std::size_t i = 0; i < fteam.size(); i++) {
			player = players[i];
			PathPoint::Ptr player_start = std::make_shared<PathPoint>(player->position());
			PathPoint::Ptr player_end = std::make_shared<PathPoint>(player->destination().first);
			std::vector<PathPoint::Ptr> add_on;
			unsigned int added_flags = 0;

			start = player_start;
			end = player_end;

			if (player->type() == AI::Flags::MoveType::CATCH) {
				added_flags = AI::Flags::FLAG_AVOID_BALL_TINY;
				double target_minus = player->MAX_RADIUS + Ball::RADIUS + 0.07;
				Point p(target_minus, 0);
				p = p.rotate(player->destination().second);
				Point ball_catch = world.ball().position();
				PathPoint::Ptr p_end = std::make_shared<PathPoint>(ball_catch - p);
				end = p_end;
				PathPoint::Ptr p_add = std::make_shared<PathPoint>(world.ball().position());
				add_on.push_back(p_add);
			}

			path.clear();
			start->g = 0.0;

			if (valid_path(player->position(), end->xy, world, player, added_flags)) {
				if (player->type() == AI::Flags::MoveType::CATCH) {
					Point diff = world.ball().position() - player->position();
					diff = diff.rotate(-player->orientation());
					Angle ang = std::max(diff.orientation(), player->orientation().angle_diff(player->destination().second));
					if (ang < Angle::ZERO) {
						ang = -ang;
					}
					if (ang < ANGLE_TOL) {
						path.push_back(std::make_pair(std::make_pair(world.ball().position(), player->destination().second), world.monotonic_time()));
						player->path(path);
						continue;
					}
				}

				path.push_back(std::make_pair(std::make_pair(end->xy, player->destination().second), world.monotonic_time()));
				for (std::vector<PathPoint::Ptr>::iterator it = add_on.begin(); it != add_on.end(); ++it) {
					path.push_back(std::make_pair(std::make_pair((*it)->xy, player->destination().second), world.monotonic_time()));
				}

				player->path(path);
				continue;
			}

			search_space.clear();
			open_set.clear();
			std::vector<Point> p = get_obstacle_boundaries(world, player, added_flags);
			std::vector<Point> end_near = get_destination_alternatives(end->xy, world, player);
			for (std::vector<Point>::iterator it = end_near.begin(); it != end_near.end(); ++it) {
				p.push_back(*it);
			}
			search_space.push_back(start);
			search_space.push_back(end);
			for (std::vector<Point>::const_iterator it = p.begin(); it != p.end(); ++it) {
				PathPoint::Ptr temp = std::make_shared<PathPoint>(*it);
				search_space.push_back(temp);
			}

			open_set.insert(start);
			bool ans = false;
			while (!open_set.empty()) {
				PathPoint::Ptr cur = *(open_set.begin());
				open_set.erase(open_set.begin());
				if (cur == end) {
					std::vector<PathPoint::Ptr> p = cur->getParents();
					for (std::vector<PathPoint::Ptr>::iterator it = p.begin(); it != p.end(); ++it) {
						if (*it != start) {
							path.push_back(std::make_pair(std::make_pair((*it)->xy, player->destination().second), world.monotonic_time()));
						}
					}
					path.push_back(std::make_pair(std::make_pair(cur->xy, player->destination().second), world.monotonic_time()));

					for (std::vector<PathPoint::Ptr>::iterator it = add_on.begin(); it != add_on.end(); ++it) {
						path.push_back(std::make_pair(std::make_pair((*it)->xy, player->destination().second), world.monotonic_time()));
					}

					ans = true;
					player->path(path);
					break;
				}
				cur->closed = true;
				for (std::vector<PathPoint::Ptr>::const_iterator it = search_space.begin(); it != search_space.end(); ++it) {
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
				std::sort(search_space.begin(), search_space.end(), [](PathPoint::Ptr a, PathPoint::Ptr b) { return (a->xy - end->xy).len() < (b->xy - end->xy).len(); });
				for (std::vector<PathPoint::Ptr>::const_iterator it1 = search_space.begin(); it1 != search_space.end(); ++it1) {
					PathPoint::Ptr cur = *it1;
					std::vector<PathPoint::Ptr> p = cur->getParents();
					if (p.size() <= 0) {
						continue;
					}
					for (std::vector<PathPoint::Ptr>::const_iterator it = p.begin(); it != p.end(); ++it) {
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

