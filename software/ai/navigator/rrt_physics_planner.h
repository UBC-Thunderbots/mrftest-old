#include "ai/navigator/rrt_planner.h"

namespace AI {
	namespace Nav {
		class PhysicsPlanner final : public RRTPlanner {
			public:
				explicit PhysicsPlanner(AI::Nav::W::World world);
				std::vector<Point> plan(AI::Nav::W::Player player, Point goal, unsigned int added_flags = 0) override;

			protected:
				/**
				 * Determines how far an endpoint in the path is from the goal location
				 */
				double distance(Glib::NodeTree<Point> *nearest, Point goal) override;

				/**
				 * This function decides how to move toward the target
				 * the gtarget is one of a random point, a waypoint, or the goal location
				 * a subclass may override this
				 */
				Point extend(AI::Nav::W::Player player, Glib::NodeTree<Point> *start, Point target) override;

			private:
				AI::Nav::W::Player curr_player;
		};
	}
}

