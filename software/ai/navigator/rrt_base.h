#include "ai/navigator/navigator.h"
#include "util/objectstore.h"
#include <glibmm.h>



namespace AI {
	namespace Nav {
		namespace RRT{

			class Waypoints : public ObjectStore::Element {
			public:
				typedef ::RefPtr<Waypoints> Ptr;
				static const std::size_t NUM_WAYPOINTS = 50;
				Point points[NUM_WAYPOINTS];
			};

			class RRTBase : public Navigator {
			private:
				void on_player_added(std::size_t i);
			protected:
				RRTBase(AI::Nav::W::World &world);
				~RRTBase();

				Waypoints::Ptr currPlayerWaypoints;
				unsigned int addedFlags;
				Point random_point();
				Point choose_target(Point goal);
				Glib::NodeTree<Point> *nearest(Glib::NodeTree<Point> *tree, Point target);
				Point extend(AI::Nav::W::Player::Ptr player, Point start, Point target);
				std::vector<Point> rrt_plan(AI::Nav::W::Player::Ptr player, Point initial, Point goal);
			};

		}
	}
}
