#include "ai/navigator/rrt_planner.h"

namespace AI{
	namespace Nav{

		class PhysicsPlanner : public RRTPlanner {
		public:
			PhysicsPlanner(AI::Nav::W::World &world);
			~PhysicsPlanner();
			std::vector<Point> plan(AI::Nav::W::Player::Ptr player, Point goal, unsigned int added_flags=0);

		protected:
			/**
			 * Determines how far an endpoint in the path is from the goal location
			 */
			double distance(Glib::NodeTree<Point> *nearest, Point goal);

			/**
			 * This function decides how to move toward the target
			 * the gtarget is one of a random point, a waypoint, or the goal locatio
			 *a subclass may override this
			 */
			Point extend(AI::Nav::W::Player::Ptr player, Glib::NodeTree<Point> *start, Point target);
		private:
			AI::Nav::W::Player::Ptr curr_player;
		};
	}
}
