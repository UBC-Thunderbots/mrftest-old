#include "ai/navigator/navigator.h"
#include "util/objectstore.h"
#include <glibmm.h>



namespace AI {
	namespace Nav {
		namespace RRT{
		/**
		 * A Navigator base class that does path planning using RRT
		 *
		 * To implement a Navigator of type RRTBase, one must:
		 * <ul>
		 * <li>Subclass RRTBase</li>
		 * <li>In the subclass, override the pure virtual function tick()</li>
		 * <li>Subclass NavigatorFactory</li>
		 * <li>In the subclass, override all the pure virtual functions</li>
		 * <li>Create an instance of the NavigatorFactory in the form of a file-scope global variable</li>
		 * </ul>
		 */
			class Waypoints : public ObjectStore::Element {
			public:
				typedef ::RefPtr<Waypoints> Ptr;
				static const std::size_t NUM_WAYPOINTS = 50;
				Point points[NUM_WAYPOINTS];
				unsigned int addedFlags;
			};

			class RRTBase : public Navigator {
			public:
				static Point empty_state();
			protected:
				RRTBase(AI::Nav::W::World &world);
				~RRTBase();

				/**
				 * Determines how far an endpoint in the path is from the goal location
				 */
				virtual double distance(Glib::NodeTree<Point> *nearest, Point goal);

				Point random_point();
				Point choose_target(Point goal, AI::Nav::W::Player::Ptr player);
				Glib::NodeTree<Point> *nearest(Glib::NodeTree<Point> *tree, Point target);
				/**
				 * This function decides how to move toward the target
				 * the gtarget is one of a random point, a waypoint, or the goal location
				 *a subclass may override this
				 */
				virtual Point extend(AI::Nav::W::Player::Ptr player, Point start, Point target);
				/**
				 * This is the useful method in this class it
				 * Generates a path for a player given the goal
				 * optional parameter post_process sets whether to try and smooth out
				 * the final path
				 */
				std::vector<Point> rrt_plan(AI::Nav::W::Player::Ptr player, Point goal, bool post_process=true);
			};

		}
	}
}
