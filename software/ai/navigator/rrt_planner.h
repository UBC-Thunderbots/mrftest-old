#include "ai/navigator/plan.h"
#include <glibmm.h>
#include "util/objectstore.h"

namespace AI {
	namespace Nav {

		//Blank struct that 
		//can be subclasses to store information 
		//in Waypoints
		struct PlayerData {
		};

		class Waypoints : public ObjectStore::Element {
		public:
			typedef ::RefPtr<Waypoints> Ptr;
			static const std::size_t NUM_WAYPOINTS = 50;
			Point points[NUM_WAYPOINTS];
			unsigned int added_flags;
			PlayerData data;
		};

		class RRTPlanner : public Plan {
		public:
			RRTPlanner(AI::Nav::W::World &world);
			~RRTPlanner();
			virtual std::vector<Point> plan(AI::Nav::W::Player::Ptr player, Point goal, unsigned int added_flags = 0);

			static Point empty_state();

		protected:

			/**
			 * Determines how far an endpoint in the path is from the goal location
			 */
			virtual double distance(Glib::NodeTree<Point> *nearest, Point goal);

			Point random_point();

			Point choose_target(Point goal, AI::Nav::W::Player::Ptr player);

			Glib::NodeTree<Point> *nearest(Glib::NodeTree<Point> *tree, Point target);

			/**
			 * This function decides how to move toward the target
			 * the target is one of a random point, a waypoint, or the goal location
			 * a subclass may override this
			 */
			virtual Point extend(AI::Nav::W::Player::Ptr player, Glib::NodeTree<Point> *start, Point target);

			/**
			 * This is the useful method in this class it
			 * Generates a path for a player given the goal
			 * optional parameter post_process sets whether to try and smooth out
			 * the final path
			 */
			std::vector<Point> rrt_plan(AI::Nav::W::Player::Ptr player, Point goal, bool post_process = true, unsigned int added_flags = 0);

		};
	}
 }
