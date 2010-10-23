#include "ai/navigator/navigator.h"
#include "tree.hh"
#include <iostream>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Flags;

namespace {
	const double MAX_SPEED = 2.0;
	const double THRESHOLD = 0.15;
	const double STEP_DISTANCE = 0.3;
	//probability that we will take a step towards the goal
	const double GOAL_PROB = 0.7;
	//number of iterations to go through for each robot until we give up and
	//just return the best partial path we've found
	const int ITERATION_LIMIT = 500;

	class RRTNavigator : public Navigator {
		public:
			NavigatorFactory &factory() const;
			void tick();
			static Navigator::Ptr create(World &world);

		private:
			RRTNavigator(World &world);
			~RRTNavigator();

			double Distance(Point nearest, Point goal);
			Point RandomPoint();
			Point ChooseTarget(Point goal);
			tree<Point>::iterator Nearest(tree<Point> *tree, Point target);
			Point EmptyState();
			Point Extend(Player::Ptr player, Point start, Point target);
			bool IsEmptyState(Point toCheck);
			std::deque<Point> RRTPlan(Player::Ptr player, Point initial, Point goal);
	};

	class RRTNavigatorFactory : public NavigatorFactory {
		public:
			RRTNavigatorFactory();
			~RRTNavigatorFactory();
			Navigator::Ptr create_navigator(World &world) const;
	};

	RRTNavigatorFactory factory_instance;

	NavigatorFactory &RRTNavigator::factory() const {
		return factory_instance;
	}

	void RRTNavigator::tick() {
		struct timespec timing;
		std::vector<std::pair<std::pair<Point, double>, timespec> > path;
		std::deque<Point> pathPoints;

		for(std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			path.clear();
			Player::Ptr player = world.friendly_team().get(i);
			timespec_now(timing);

			const double dist = (player->position() - player->destination().first).len();
			timing.tv_sec += dist / MAX_SPEED;

			pathPoints.clear();
			pathPoints = RRTPlan(player, player->position(), player->destination().first);

			double destOrientation = player->destination().second;
			for(std::size_t i = 0; i < pathPoints.size(); ++i) {
				//the last point will just use whatever the last orientation was
				if(i + 1 != pathPoints.size())
					destOrientation = (pathPoints[i + 1] - pathPoints[i]).orientation();

				path.push_back(std::make_pair(std::make_pair(pathPoints[i], destOrientation), timing));
			}

			//just use the current player position as the destination if we are within the
			//threshold already
			if(pathPoints.size() == 0)
				path.push_back(std::make_pair(std::make_pair(player->position(), 0), timing));

			player->path(path);
		}
	}

	double RRTNavigator::Distance(Point nearest, Point goal) {
		return (goal - nearest).len();
	}

	Point RRTNavigator::EmptyState() {
		return Point(-10000, -10000);
	}

	bool RRTNavigator::IsEmptyState(Point toCheck) {
		return toCheck.x == EmptyState().x && toCheck.y && EmptyState().y;
	}

	//generate a random point from the field
	Point RRTNavigator::RandomPoint() {
		return Point(rand() % static_cast<int>(world.field().width()), rand() % static_cast<int>(world.field().length()));
	}

	//choose a target to extend toward, the goal with GOAL_PROB or a random point
	Point RRTNavigator::ChooseTarget(Point goal) {
		double p = rand() / double(RAND_MAX);

		if(p > 0 && p < GOAL_PROB)
			return goal;
		else
			return RandomPoint();
	}

	//finds the point in the tree that is nearest to the target point
	tree<Point>::iterator RRTNavigator::Nearest(tree<Point> *rrtTree, Point target) {
		tree<Point>::iterator it = rrtTree->begin();
		tree<Point>::iterator end = rrtTree->end();

		tree<Point>::pre_order_iterator nearest = it;
		while(it != end) {
			if(Distance(*it, target) < Distance(*nearest, target)) {
				nearest = it;
			}
			++it;
		}

		return nearest;
	}

	//extend by STEP_DISTANCE towards the target from the start
	Point RRTNavigator::Extend(Player::Ptr player, Point start, Point target) {
		Point extendPoint = start + ((target - start).norm() * STEP_DISTANCE);
		//check if the point is invalid (collision, out of bounds, etc...)
		//if it does then return EmptyState()

		return extendPoint;
	}

	std::deque<Point> RRTNavigator::RRTPlan(Player::Ptr player, Point initial, Point goal) {
		Point nearest, extended, target;
		tree<Point>::iterator nearestNode, lastAdded;
		tree<Point> rrtTree;

		nearest = initial;
		rrtTree.set_head(initial);
		lastAdded = rrtTree.begin();

		int iterationCounter = 0;
		while(Distance(nearest, goal) > THRESHOLD && iterationCounter < ITERATION_LIMIT) {
			target = ChooseTarget(goal);
			nearestNode = Nearest(&rrtTree, target);
			nearest = *nearestNode;
			extended = Extend(player, nearest, target);

			if(!IsEmptyState(extended)) {
				lastAdded = rrtTree.append_child(nearestNode, extended);
			}

			iterationCounter++;
		}

		if(iterationCounter == ITERATION_LIMIT)
			std::cout << "Reached limit, path not found" << std::endl;

		//calculations complete, trace backwards to get the points in the path
		tree<Point>::iterator itPath = lastAdded;

		//stores the final path of points
		std::deque<Point> pathPoints;
		pathPoints.push_front(*lastAdded);

		while(lastAdded != rrtTree.begin()) {
			lastAdded = rrtTree.parent(lastAdded);
			pathPoints.push_front(*lastAdded);
		}
		//remove the front of the list, this is the starting point
		pathPoints.pop_front();
		return pathPoints;
	}

	Navigator::Ptr RRTNavigator::create(World &world) {
		const Navigator::Ptr p(new RRTNavigator(world));
		return p;
	}

	RRTNavigator::RRTNavigator(World &world) : Navigator(world) {
	}

	RRTNavigator::~RRTNavigator() {
	}

	RRTNavigatorFactory::RRTNavigatorFactory() : NavigatorFactory("RRT Navigator") {
	}

	RRTNavigatorFactory::~RRTNavigatorFactory() {
	}

	Navigator::Ptr RRTNavigatorFactory::create_navigator(World &world) const {
		return RRTNavigator::create(world);
	}
}
