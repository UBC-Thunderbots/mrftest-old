#include "ai/navigator/navigator.h"
#include "ai/navigator/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;

namespace {
	const double MAX_SPEED = 2.0;
	const double THRESHOLD = 0.15;
	const double STEP_DISTANCE = 0.3;
	// probability that we will take a step towards the goal
	const double GOAL_PROB = 0.7;
	// number of iterations to go through for each robot until we give up and
	// just return the best partial path we've found
	const int ITERATION_LIMIT = 2000;

	class rrt_navigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			void tick();
			static Navigator::Ptr create(World &world);

		private:
			rrt_navigator(World &world);
			~rrt_navigator();

			double Distance(Point nearest, Point goal);
			Point RandomPoint();
			Point ChooseTarget(Point goal);
			NodeTree<Point> *Nearest(NodeTree<Point> *tree, Point target);
			Point EmptyState();
			Point Extend(Player::Ptr player, Point start, Point target);
			bool IsEmptyState(Point toCheck);
			std::deque<Point> RRTPlan(Player::Ptr player, Point initial, Point goal);
	};

	class rrt_navigatorFactory : public NavigatorFactory {
		public:
			rrt_navigatorFactory();
			~rrt_navigatorFactory();
			Navigator::Ptr create_navigator(World &world) const;
	};

	rrt_navigatorFactory factory_instance;

	NavigatorFactory &rrt_navigator::factory() const {
		return factory_instance;
	}

	void rrt_navigator::tick() {
		struct timespec timing;
		std::vector<std::pair<std::pair<Point, double>, timespec> > path;
		std::deque<Point> pathPoints;

		for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			path.clear();
			Player::Ptr player = world.friendly_team().get(i);
			timespec_now(timing);

			const double dist = (player->position() - player->destination().first).len();
			timing.tv_sec += dist / MAX_SPEED;

			pathPoints.clear();
			pathPoints = RRTPlan(player, player->position(), player->destination().first);

			double destOrientation = player->destination().second;
			for (std::size_t j = 0; j < pathPoints.size(); ++j) {
				// the last point will just use whatever the last orientation was
				if (j + 1 != pathPoints.size()) {
					destOrientation = (pathPoints[j + 1] - pathPoints[j]).orientation();
				}

				path.push_back(std::make_pair(std::make_pair(pathPoints[j], destOrientation), timing));
			}

			// just use the current player position as the destination if we are within the
			// threshold already
			if (pathPoints.size() == 0) {
				path.push_back(std::make_pair(std::make_pair(player->position(), destOrientation), timing));
			}

			player->path(path);
		}
	}

	double rrt_navigator::Distance(Point nearest, Point goal) {
		return (goal - nearest).len();
	}

	Point rrt_navigator::EmptyState() {
		return Point(-10000, -10000);
	}

	bool rrt_navigator::IsEmptyState(Point toCheck) {
		return toCheck.x == EmptyState().x && toCheck.y && EmptyState().y;
	}

	// generate a random point from the field
	Point rrt_navigator::RandomPoint() {
		double randomX = ((std::rand() % static_cast<int>(world.field().length() * 100)) - (world.field().length() * 50)) / 100;
		double randomY = ((std::rand() % static_cast<int>(world.field().width() * 100)) - (world.field().width() * 50)) / 100;

		return Point(randomX, randomY);
	}

	// choose a target to extend toward, the goal with GOAL_PROB or a random point
	Point rrt_navigator::ChooseTarget(Point goal) {
		double p = rand() / double(RAND_MAX);

		if (p > 0 && p < GOAL_PROB) {
			return goal;
		} else {
			return RandomPoint();
		}
	}

	// finds the point in the tree that is nearest to the target point
	NodeTree<Point> *rrt_navigator::Nearest(NodeTree<Point> *rrtTree, Point target) {
		NodeTree<Point> *nearest = rrtTree;
		NodeTree<Point> *currNode;

		std::vector<NodeTree<Point> *> nodeQueue;
		nodeQueue.push_back(rrtTree);

		// iterator through all the nodes in the tree, finding which is closest to the target
		while (nodeQueue.size() > 0) {
			currNode = nodeQueue.back();
			nodeQueue.pop_back();

			if (Distance(currNode->data(), target) < Distance(nearest->data(), target)) {
				nearest = currNode;
			}

			for (unsigned int i = 0; i < currNode->child_count(); ++i) {
				nodeQueue.push_back(currNode->nth_child(i));
			}
		}

		return nearest;
	}

	// extend by STEP_DISTANCE towards the target from the start
	Point rrt_navigator::Extend(Player::Ptr player, Point start, Point target) {
		Point extendPoint = start + ((target - start).norm() * STEP_DISTANCE);
		// check if the point is invalid (collision, out of bounds, etc...)
		// if it is then return EmptyState()

		if (!valid_path(start, extendPoint, world, player)) {
			return EmptyState();
		}

		return extendPoint;
	}

	std::deque<Point> rrt_navigator::RRTPlan(Player::Ptr player, Point initial, Point goal) {
		Point nearest, extended, target;

		NodeTree<Point> *nearestNode;
		NodeTree<Point> *lastAdded;
		NodeTree<Point> rrtTree(initial);

		nearest = initial;
		lastAdded = &rrtTree;

		int iterationCounter = 0;
		while (Distance(nearest, goal) > THRESHOLD && iterationCounter < ITERATION_LIMIT) {
			target = ChooseTarget(goal);
			nearestNode = Nearest(&rrtTree, target);
			nearest = nearestNode->data();
			extended = Extend(player, nearest, target);

			if (!IsEmptyState(extended)) {
				lastAdded = nearestNode->append_data(extended);
			}

			iterationCounter++;
		}

		if (iterationCounter == ITERATION_LIMIT) {
			// LOG_WARN("Reached limit, path not found");
		}

		// stores the final path of points
		std::deque<Point> pathPoints;
		pathPoints.push_front(lastAdded->data());

		// calculations complete, trace backwards to get the points in the path
		const NodeTree<Point> *iterator = lastAdded;

		while (iterator != &rrtTree) {
			iterator = iterator->parent();
			pathPoints.push_front(iterator->data());
		}

		// remove the front of the list, this is the starting point
		pathPoints.pop_front();
		return pathPoints;
	}

	Navigator::Ptr rrt_navigator::create(World &world) {
		const Navigator::Ptr p(new rrt_navigator(world));
		return p;
	}

	rrt_navigator::rrt_navigator(World &world) : Navigator(world) {
	}

	rrt_navigator::~rrt_navigator() {
	}

	rrt_navigatorFactory::rrt_navigatorFactory() : NavigatorFactory("RRT Navigator") {
	}

	rrt_navigatorFactory::~rrt_navigatorFactory() {
	}

	Navigator::Ptr rrt_navigatorFactory::create_navigator(World &world) const {
		return rrt_navigator::create(world);
	}
}

